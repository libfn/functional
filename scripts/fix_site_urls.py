#!/usr/bin/env python3
"""Repair the links znai writes into the deployed site.

Two repairs, both for want of anywhere earlier to make them.

znai builds every internal URL as `/<doc-id>/<chapter>/<page>`. The site is deployed at the root
of its domain, so the doc id is empty and the URL arrives doubled, as `//chapter/page` - which a
browser reads as a host name rather than a path, sending the reader off the site entirely. The
navigation escapes this because it is rendered by script that never follows the href; a link
written in a page does not. Collapsing the pair to one slash restores the path znai meant.

`View on GitHub` is built by the browser as the site's base link followed by the page's own
`viewOnRelativePath`, or by its chapter and file name when that is absent - and absent is all it
ever is, nothing on znai's side populating it. Left alone, a page generated from a root document
would point at a file that does not exist, so each page is given the path it was rendered from.
"""
from __future__ import annotations

import json
import pathlib
import re
import sys

import stage_docs_source as staging

MARKUP = (".html", ".json", ".js")

# Only where znai spells an internal target: an external one carries its scheme, so `https://`
# is never matched.
DOUBLED = re.compile(r'((?:"url" ?: ?|href=)")//')

# A serialized toc item, wherever it appears: in the page, in the navigation, in the page index.
TOC_ITEM = re.compile(
    r'("dirName"\s*:\s*"([^"]*)",\s*"fileName"\s*:\s*"([^"]*)",\s*'
    r'"fileExtension"\s*:\s*"[^"]*",\s*"viewOnRelativePath"\s*:\s*)null')


def source_of(chapter: str, page: str) -> str | None:
    """The repository file a page was rendered from, or None where there is no one file."""
    for document, directory, _split in staging.DOCUMENTS:
        if directory == chapter:
            return document
    return f"docs/{chapter}/{page}.md" if chapter else None


def main() -> None:
    if len(sys.argv) != 2:
        sys.stderr.write("usage: fix_site_urls.py <deployed site directory>\n")
        sys.exit(2)

    site = pathlib.Path(sys.argv[1])
    if not site.is_dir():
        sys.stderr.write(f"Error: {site} is not a deployed site directory\n")
        sys.exit(2)

    def point_at_source(match: re.Match[str]) -> str:
        source = source_of(match.group(2), match.group(3))
        return match.group(1) + (json.dumps(source) if source else "null")

    links = pages = 0
    for path in site.rglob("*"):
        if path.suffix not in MARKUP or not path.is_file():
            continue
        original = path.read_text(encoding="utf-8")

        text, count = DOUBLED.subn(r"\1/", original)
        links += count
        text, count = TOC_ITEM.subn(point_at_source, text)
        pages += count

        if text != original:
            path.write_text(text, encoding="utf-8")

    if not links:
        sys.stderr.write("Error: no doubled internal links found; has znai stopped doubling them?\n")
        sys.exit(2)
    if not pages:
        sys.stderr.write("Error: no page was pointed at its source; has znai started doing it?\n")
        sys.exit(2)
    print(f"repaired {links} internal links, pointed {pages} pages at their source")


if __name__ == "__main__":
    main()
