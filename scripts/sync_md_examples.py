#!/usr/bin/env python3
"""Keep markdown code fences in sync with the compiled C++ examples they quote.

A fence is quoted from a named region of an example: the example brackets the region with a pair of
`// sync-example-<name>` comments, the document places `<!-- sync-example-<name> -->` immediately
above the ```cpp fence receiving it. A fence inside a blockquote keeps its `> ` prefix. Every
region must be quoted, so a broken anchor fails rather than letting the fence drift.
"""
from __future__ import annotations

import argparse
import pathlib
import re
import sys

# Markdown document -> the compiled example its fences quote. Keep the `files` pattern of the
# sync-md-examples hook in .pre-commit-config.yaml covering exactly these paths.
DOCUMENTS = {
    "README.md": "examples/readme/main.cpp",
    "TYPE_ALGEBRA.md": "examples/type_algebra/main.cpp",
}

BOUNDARY = "// sync-example-"

# Blockquote prefix, anchor comment, and the fence it introduces, body captured non-greedily.
FENCE = re.compile(
    r"^([ >]*)<!-- sync-example-([\w-]+) -->[ \t]*\n[ >]*```cpp\n(.*?)\n[ >]*```[ \t]*$",
    re.MULTILINE | re.DOTALL,
)


def fail(message: str) -> None:
    sys.stderr.write(f"Error: {message}\n")
    sys.exit(2)


def read(path: pathlib.Path, repo: pathlib.Path) -> str:
    if not path.exists():
        fail(f"{path.relative_to(repo)} does not exist")
    return path.read_text(encoding="utf-8")


def extract(example: pathlib.Path, repo: pathlib.Path) -> dict[str, list[str]]:
    """Collect the regions an example offers for quotation, keyed by name."""
    where = example.relative_to(repo)
    regions: dict[str, list[str]] = {}
    open_name: str | None = None
    body: list[str] = []

    for lineno, line in enumerate(read(example, repo).splitlines(), start=1):
        boundary = line.strip()
        if not boundary.startswith(BOUNDARY):
            if open_name is not None:
                body.append(line)
            continue
        name = boundary[len(BOUNDARY) :]
        if open_name is None:
            if name in regions:
                fail(f"{where}:{lineno}: region {name!r} opened again after it was closed")
            open_name, body = name, []
        elif name == open_name:
            regions[open_name] = body
            open_name = None
        else:
            fail(f"{where}:{lineno}: region {open_name!r} closed by {name!r}")

    if open_name is not None:
        fail(f"{where}: region {open_name!r} is never closed")
    if not regions:
        fail(f"{where} offers no regions to quote")
    return regions


def sync(document: pathlib.Path, example: pathlib.Path, repo: pathlib.Path) -> bool:
    """Rewrite the document's anchored fences from the example; report whether anything changed."""
    regions = extract(example, repo)
    quoted: set[str] = set()

    def quote(match: re.Match) -> str:
        prefix, name, _ = match.groups()
        if name not in regions:
            fail(f"{document.relative_to(repo)}: no region {name!r} in {example.relative_to(repo)}")
        quoted.add(name)
        body = "\n".join(f"{prefix}{line}".rstrip() for line in regions[name])
        return f"{prefix}<!-- sync-example-{name} -->\n{prefix}```cpp\n{body}\n{prefix}```"

    text = read(document, repo)
    updated = FENCE.sub(quote, text)

    for name in sorted(set(regions) - quoted):
        fail(f"region {name!r} of {example.relative_to(repo)} is never quoted by "
             f"{document.relative_to(repo)}")

    if text == updated:
        return False
    document.write_text(updated, encoding="utf-8")
    print(f"synced {document.relative_to(repo)} <- {example.relative_to(repo)}")
    return True


def main() -> None:
    parser = argparse.ArgumentParser(description="Synchronize markdown files with compiled C++ examples.")
    parser.add_argument(
        "documents",
        nargs="*",
        default=list(DOCUMENTS),
        help=f"documents to synchronize, any of: {', '.join(DOCUMENTS)} (default: all)",
    )
    args = parser.parse_args()

    repo = pathlib.Path(__file__).resolve().parents[1]
    changed = False
    for name in args.documents:
        if name not in DOCUMENTS:
            fail(f"unknown document {name!r}, expected one of: {', '.join(DOCUMENTS)}")
        changed |= sync(repo / name, repo / DOCUMENTS[name], repo)

    if changed:
        sys.exit(1)

    print(f"All code examples in {', '.join(args.documents)} are fully synchronized!")


if __name__ == "__main__":
    main()
