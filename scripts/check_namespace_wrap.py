#!/usr/bin/env python3
"""Check that every fn/pfn namespace in include/ carries its ABI inline namespace.

The layer rule: fn opens the mode-sensitive spelling (inline namespace
LIBFN_VERSION), pfn the mode-less one (inline namespace LIBFN_VERSION_BASE) —
pfn layouts never depend on the C++26 type ordering, so pfn types stay
link-compatible across modes. An unwrapped opening silently lands its entities
outside the versioned ABI namespace: every source-tree build stays green, while
the entities' mangled names escape the version and break ABI coexistence of
different library versions in one program. Any fn/pfn opening spelled
non-canonically (indented, missing brace, trailing comment, wrong macro for the
layer) is rejected outright rather than matched tolerantly:
cmake/StripNamespaceWrap.cmake strips the wrap for the docs build by these
exact literals, so the canonical spelling is itself the invariant. Run as a
pre-commit hook: exits non-zero naming each offending opening as file:line.
"""
import pathlib
import re
import sys

OPENING_RE = re.compile(r"\s*namespace\s+(fn|pfn)\b")
INLINE_OF = {
    "fn": "inline namespace LIBFN_VERSION {",
    "pfn": "inline namespace LIBFN_VERSION_BASE {",
}
NESTED_OF = {
    "fn": "namespace fn::inline LIBFN_VERSION::",
    "pfn": "namespace pfn::inline LIBFN_VERSION_BASE::",
}

repo = pathlib.Path(__file__).resolve().parents[1]
include = repo / "include"

errors = []
for path in sorted(include.rglob("*.hpp")):
    lines = path.read_text().splitlines()
    for lineno, line in enumerate(lines, start=1):
        match = OPENING_RE.match(line)
        if not match:
            continue
        root = match.group(1)
        if line == f"namespace {root} {{":
            if lineno >= len(lines) or lines[lineno] != INLINE_OF[root]:
                errors.append((path, lineno, line))
        elif not line.startswith(NESTED_OF[root]):
            errors.append((path, lineno, line))

for path, lineno, line in errors:
    print(
        f"{path.relative_to(repo).as_posix()}:{lineno}: '{line}' does not open inline namespace"
        " LIBFN_VERSION: its entities would land outside the versioned ABI namespace"
    )
sys.exit(1 if errors else 0)
