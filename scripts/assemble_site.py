#!/usr/bin/env python3
"""Assemble the complete website tree from a fresh docs build and the archived versions.

The archive (libfn/website, its site/ directory) carries the root of the site - the latest
release's documentation - alongside one directory per released version. A docs build produces
the root and the current version's directory only, so this replaces the archived root with the
fresh one, adds the fresh version directory, keeps every other version untouched, and writes
versions.html - the index of every version the assembled site serves.
"""
from __future__ import annotations

import argparse
import html
import pathlib
import re
import shutil
import sys

# scripts/sync_versions.py's canonical SemVer grammar, minus build metadata (it cannot name a
# version directory), plus the v prefix.
VERSION_DIR = re.compile(
    r"^v(0|[1-9]\d*)\.(0|[1-9]\d*)\.(0|[1-9]\d*)"
    r"(?:-((?:0|[1-9]\d*|\d*[a-zA-Z-][0-9a-zA-Z-]*)(?:\.(?:0|[1-9]\d*|\d*[a-zA-Z-][0-9a-zA-Z-]*))*))?$")

PAGE = """<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>libfn documentation versions</title>
<style>
body {{ font-family: system-ui, sans-serif; margin: 3em auto; max-width: 40em; padding: 0 1em; }}
h1 {{ font-size: 1.4em; }}
li {{ margin: 0.3em 0; }}
.pre {{ opacity: 0.6; }}
.hpp {{ font-size: 0.85em; margin-left: 0.7em; }}
</style>
</head>
<body>
<h1>libfn documentation versions</h1>
<p><a href="/">latest ({latest})</a></p>
<ul>
{items}
</ul>
</body>
</html>
"""


def fail(message: str) -> None:
    sys.stderr.write(f"Error: {message}\n")
    sys.exit(2)


def version_key(name: str):
    """Order version directories newest-first under sorted(reverse=True).

    A release outranks its own pre-releases; pre-release identifiers compare with their
    numeric runs as numbers, so rc9 precedes rc10 (SemVer's lexical rule would misorder
    them, and the project's tags never rely on the difference).
    """
    match = VERSION_DIR.match(name)
    numbers = [int(match.group(index)) for index in (1, 2, 3)]
    prerelease = match.group(4)
    if prerelease is None:
        return (numbers, 1, [])
    runs = [(0, int(run), "") if run.isdigit() else (1, 0, run)
            for identifier in prerelease.split(".")
            for run in re.findall(r"[0-9]+|[^0-9]+", identifier)]
    return (numbers, 0, runs)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("archive", type=pathlib.Path, help="the archived site/ directory")
    parser.add_argument("fresh", type=pathlib.Path, help="freshly built docs directory")
    parser.add_argument("--version-file", type=pathlib.Path, default=pathlib.Path("VERSION"),
                        help="file naming the version the fresh build was made from")
    args = parser.parse_args()

    try:
        raw = args.version_file.read_text(encoding="utf-8").strip()
    except OSError as error:
        fail(f"cannot read the version: {error}")
    version = "v" + raw
    if not VERSION_DIR.match(version):
        fail(f"{args.version_file}: '{raw}' is not a releasable version"
             + ("; build metadata cannot name a version directory" if "+" in raw else ""))
    if not (args.archive / "index.html").is_file():
        fail(f"{args.archive} is not the archived site")
    for name in ("index.html", "libfn.hpp", f"{version}/index.html", f"{version}/libfn.hpp"):
        if not (args.fresh / name).is_file():
            fail(f"{args.fresh}/{name} missing; is {args.fresh} a complete docs build?")
    for entry in args.fresh.iterdir():
        if entry.is_dir() and VERSION_DIR.match(entry.name) and entry.name != version:
            fail(f"{args.fresh}/{entry.name} does not belong to {version}; stale build tree?")

    kept = [entry.name for entry in args.archive.iterdir()
            if entry.is_dir() and VERSION_DIR.match(entry.name) and entry.name != version]
    for name in kept:
        for probe in ("index.html", "libfn.hpp"):
            if not (args.archive / name / probe).is_file():
                fail(f"archived {name} lacks {probe}; damaged archive?")
    for entry in args.archive.iterdir():
        if entry.name in kept:
            continue
        if entry.name == version:
            print(f"replacing already archived {version}")
        shutil.rmtree(entry) if entry.is_dir() else entry.unlink()
    shutil.copytree(args.fresh, args.archive, dirs_exist_ok=True)

    versions = sorted([*kept, version], key=version_key, reverse=True)
    items = "\n".join(
        f'<li><a href="/{name}/">{name}</a>'
        + ("" if VERSION_DIR.match(name).group(4) is None else ' <span class="pre">pre-release</span>')
        + f' <a class="hpp" href="/{name}/libfn.hpp">single header</a></li>'
        for name in map(html.escape, versions))
    (args.archive / "versions.html").write_text(
        PAGE.format(latest=html.escape(version), items=items), encoding="utf-8")

    print(f"assembled site: root {version}, versions: {', '.join(versions)}")


if __name__ == "__main__":
    main()
