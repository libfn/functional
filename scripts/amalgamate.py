#!/usr/bin/env python3
"""Generate a single self-contained header from the include/ tree.

Intended for online compilers (Compiler Explorer, Wandbox) and standalone bug
reproducers, where adding an include path is not an option. The output is a
release artifact, not a supported way to consume the library: prefer the real
headers, which give real paths in diagnostics.

The expansion rule is derived from the headers themselves rather than from a
hand-kept list, so it stays correct as headers are added:

  - a header carrying an `#ifndef INCLUDE_*` guard is emitted at its first
    inclusion and skipped afterwards, exactly as the preprocessor would;
  - a header without a guard is emitted at *every* inclusion site. This is not
    an edge case to tolerate but the reason the rule exists: fn/detail/
    macro_begin.hpp and macro_end.hpp are deliberately guardless, and bracket
    the 23 headers that use FWD / DEDUCED_RETURN. Emitting them once would
    leave FWD undefined after the first macro_end.

Guards are kept in the output (harmless, and the result stays idempotent under
double inclusion). Standard-library includes at a header's top level — outside
any conditional block other than the include guard — are hoisted and
deduplicated; inside a conditional block they stay in place, where the
preprocessor evaluates them in context. LIBFN_CXX26 is never baked in:
libfn_version.hpp is inlined verbatim, so one artifact serves both modes,
selected as usual by defining the macro.

The rewrites recognize the includes these headers use, conservatively: a local
include the script cannot resolve or cannot safely relocate, and any
conditional structure it cannot follow, fail the run rather than guess their
way into the artifact.

Usage: scripts/amalgamate.py [-o OUTPUT] [--revision SHA]
"""
import argparse
import pathlib
import re
import subprocess
import sys

repo = pathlib.Path(__file__).resolve().parents[1]
include = repo / "include"
cmakelists = include / "CMakeLists.txt"

# The install file sets are the source of truth for what ships; check_install_headers.py
# keeps them honest against the tree. Public roots are everything not under a detail/.
LISTED_RE = re.compile(r"^\s+((?:pfn|fn)/\S+\.hpp|libfn_version\.hpp)\s*$", re.MULTILINE)
GUARD_RE = re.compile(r"^#ifndef (INCLUDE_\w+)\r?\n#define \1\s*$", re.MULTILINE)
LOCAL_INCLUDE_RE = re.compile(r'^#include ["<]((?:pfn|fn)/[^">]+\.hpp|libfn_version\.hpp)[">]\s*$')
# libfn_version.hpp is spelled without a directory, so exclude it explicitly.
SYSTEM_INCLUDE_RE = re.compile(r"^#include <(?!libfn_version\.hpp)([^/>]+)>\s*$")
BANNER_RE = re.compile(r"\A(?://[^\n]*\n)+\n")  # per-file ISC notice; LICENSE.md at the top covers them all
COND_OPEN_RE = re.compile(r"^\s*#\s*(?:if|ifdef|ifndef)\b")
COND_CLOSE_RE = re.compile(r"^\s*#\s*endif\b")
# Any local-include-looking line the strict recognizer does not match must fail loudly.
LOCAL_SUSPECT_RE = re.compile(r'^\s*#\s*include\s*["<](?:(?:pfn|fn)/|libfn_version\.hpp)')

LICENCE = """// libfn — single-header amalgamation of https://github.com/libfn/functional
//
{licence}
//
// GENERATED FILE — DO NOT EDIT. Regenerate with scripts/amalgamate.py.
// Version: {version}
// Revision: {revision}
"""


def read(header: str) -> str:
    path = include / header
    if not path.is_file():
        sys.stderr.write(f"{header}: included but not found under include/\n")
        sys.exit(1)
    return path.read_text()


def public_roots() -> list[str]:
    listed = set(LISTED_RE.findall(cmakelists.read_text()))
    if not listed:
        sys.stderr.write(f"{cmakelists.relative_to(repo)}: no header file sets found\n")
        sys.exit(1)
    return sorted(h for h in listed if "/detail/" not in h)


def expand(header: str, emitted: set[str], stack: tuple[str, ...], system: set[str], out: list[str]) -> None:
    """Depth-first expansion reproducing the preprocessor's order for `header`."""
    if header in stack:
        sys.stderr.write(f"{header}: include cycle via {' -> '.join(stack)}\n")
        sys.exit(1)
    text = read(header)
    guarded = bool(GUARD_RE.search(text))
    if guarded and header in emitted:
        return
    emitted.add(header)

    text = BANNER_RE.sub("", text)
    out.append(f"\n// ---------- BEGIN {header} ----------\n")
    # Hoisting an include out of a conditional block would change what the
    # preprocessor sees, so hoist only at the file's baseline depth — inside the
    # include guard and nothing else; deeper system includes stay in place.
    baseline = 1 if guarded else 0
    depth = 0
    kept: list[str] = []
    for lineno, line in enumerate(text.splitlines(), 1):
        if COND_OPEN_RE.match(line):
            depth += 1
        elif COND_CLOSE_RE.match(line):
            depth -= 1
        elif local := LOCAL_INCLUDE_RE.match(line):
            if depth != baseline:
                sys.stderr.write(f"{header}:{lineno}: local include inside a conditional block\n")
                sys.exit(1)
            out.append("\n".join(kept))
            kept = []
            expand(local.group(1), emitted, stack + (header,), system, out)
            out.append(f"\n// ---------- RESUME {header} ----------\n")
            continue
        elif found := SYSTEM_INCLUDE_RE.match(line):
            if depth == baseline:
                system.add(found.group(1))
                continue
        elif LOCAL_SUSPECT_RE.match(line):
            sys.stderr.write(f"{header}:{lineno}: unrecognized local include spelling\n")
            sys.exit(1)
        kept.append(line)
    if depth != 0:
        sys.stderr.write(f"{header}: unbalanced preprocessor conditionals\n")
        sys.exit(1)
    out.append("\n".join(kept))
    out.append(f"\n// ---------- END {header} ----------\n")


def revision() -> str:
    try:
        described = subprocess.run(
            ["git", "-C", str(repo), "describe", "--tags", "--always", "--dirty"],
            capture_output=True,
            text=True,
            check=True,
        )
        return described.stdout.strip()
    except (OSError, subprocess.CalledProcessError):
        return "unknown"


parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("-o", "--output", type=pathlib.Path, help="write here instead of stdout")
parser.add_argument("--revision", help="provenance string; defaults to git describe")
args = parser.parse_args()

emitted: set[str] = set()
system: set[str] = set()
body: list[str] = []
for root in public_roots():
    expand(root, emitted, (), system, body)

version = (repo / "VERSION").read_text().strip()
licence = "\n".join(("// " + line).rstrip() for line in (repo / "LICENSE.md").read_text().splitlines())
document = [
    LICENCE.format(licence=licence, version=version, revision=args.revision or revision()),
    "\n#ifndef INCLUDE_LIBFN_AMALGAMATED\n#define INCLUDE_LIBFN_AMALGAMATED\n\n",
    "".join(f"#include <{h}>\n" for h in sorted(system)),
    "".join(body),
    "\n#endif // INCLUDE_LIBFN_AMALGAMATED\n",
]
text = re.sub(r"\n{3,}", "\n\n", "".join(document))

if args.output:
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(text)
else:
    sys.stdout.write(text)
