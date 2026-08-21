#!/usr/bin/env python3
"""Repair the links znai writes into the deployed site.

znai builds every internal URL as `/<doc-id>/<chapter>/<page>` and deploys into a directory
named after the doc id. The site root is generated with an empty doc id, so its URLs arrive
doubled - `//chapter/page`, which a browser reads as a host name - and the pair is collapsed
to one slash. Each released version's copy is generated with the version directory as its doc
id, so its URLs arrive with exactly the prefix that serving it under `/v<version>/` needs and
nothing is collapsed; a doubled link there means znai dropped the prefix, and fails the build.

In either tree a redirect stub's target gains a trailing slash: the stub sits at
`<chapter>/index.html`, which GitHub Pages serves for the extension-less `.../<chapter>/index`
ahead of the real page at `<chapter>/index/index.html`, so the target alone would redirect the
stub to itself. And each page is given the repository path it was rendered from: znai never
populates `viewOnRelativePath`, leaving the browser to build a `View on GitHub` link that
points at a file that does not exist. In a versioned tree the GitHub source links are also
pinned to the release's tag - left on `main`, a permanent page's links would drift as the
repository evolves.
"""
from __future__ import annotations

import argparse
import json
import pathlib
import re
import sys

import stage_docs_source as staging

MARKUP = (".html", ".json", ".js", ".xml")

# Only where znai spells an internal target: an external one carries its scheme, so `https://`
# is never matched.
DOUBLED = re.compile(r'((?:"url" ?: ?|href=)")//')

# A chapter redirect stub's meta-refresh target; the trailing slash keeps the repaired stub
# from shadowing its own destination (see the module docstring).
STUB = re.compile(r'(content="0; url=)//([^"]+?)/?"')

# A search index entry, as search-entries.xml spells it.
ENTRY = re.compile(r"(<url>)//")

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
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("site", type=pathlib.Path, help="deployed site directory")
    parser.add_argument("--doc-id", default="",
                        help="doc id the tree was generated with; empty for the site root")
    args = parser.parse_args()

    site = args.site
    if not site.is_dir():
        sys.stderr.write(f"Error: {site} is not a deployed site directory\n")
        sys.exit(2)

    doc_id = args.doc_id
    if doc_id:
        prefixed = re.escape(doc_id)
        # src= joins the scan here only: under a doc id znai prefixes asset URLs too, and a
        # script source leaking across versions is the quietest possible failure - while with
        # the empty id it emits them single-slash, leaving root mode nothing to collapse.
        attrs = r'(?:"url" ?: ?|href=|src=)'
        link = re.compile(rf'({attrs}")/{prefixed}/')
        stub = re.compile(rf'(content="0; url=/{prefixed}/)([^"]+?)/?"')
        entry = re.compile(rf"<url>/{prefixed}/")
        doubled_link = re.compile(rf'({attrs}")//')
        # A root-absolute link without the prefix would silently serve the reader another
        # version's page; a GitHub source link left on main would drift as the repo evolves,
        # while the doc id names the exact ref this tree documents.
        unprefixed = re.compile(rf'({attrs}")/(?!{prefixed}["/]|/)')
        github_main = re.compile(r"(https://github\.com/libfn/functional/(?:blob|tree)/)main\b")
    else:
        link, stub, entry = DOUBLED, STUB, ENTRY

    def point_at_source(match: re.Match[str]) -> str:
        source = source_of(match.group(2), match.group(3))
        return match.group(1) + (json.dumps(source) if source else "null")

    links = stubs = entries = pages = doubled = stray = refs = 0
    for path in site.rglob("*"):
        if path.suffix not in MARKUP or not path.is_file():
            continue
        original = path.read_text(encoding="utf-8")

        if doc_id:
            doubled += len(doubled_link.findall(original))
            stray += len(unprefixed.findall(original))
            links += len(link.findall(original))
            entries += len(entry.findall(original))
            text, count = stub.subn(r'\1\2/"', original)
            stubs += count
            text, count = github_main.subn(rf"\g<1>{doc_id}", text)
            refs += count
        else:
            text, count = link.subn(r"\1/", original)
            links += count
            text, count = stub.subn(r'\1/\2/"', text)
            stubs += count
            text, count = entry.subn(r"\1/", text)
            entries += count
        text, count = TOC_ITEM.subn(point_at_source, text)
        pages += count

        if text != original:
            path.write_text(text, encoding="utf-8")

    if doubled:
        sys.stderr.write(f"Error: {doubled} doubled internal links found; znai dropped the doc id,"
                         " or a page carries a protocol-relative external link\n")
        sys.exit(2)
    if doc_id and stray:
        sys.stderr.write(f"Error: {stray} internal links lack the /{doc_id}/ prefix and would"
                         " serve another version's pages\n")
        sys.exit(2)
    if doc_id and not refs:
        sys.stderr.write("Error: no GitHub source link was pinned; has the viewOn base moved"
                         " off blob/main?\n")
        sys.exit(2)
    if not links:
        sys.stderr.write("Error: no doubled internal links found; has znai stopped doubling them?\n"
                         if not doc_id else
                         f"Error: no internal link carries /{doc_id}/; has znai dropped the doc id?\n")
        sys.exit(2)
    if not stubs:
        sys.stderr.write("Error: no redirect stub was repaired; has znai stopped doubling its targets?\n"
                         if not doc_id else
                         "Error: no redirect stub was repaired; has znai stopped emitting stubs?\n")
        sys.exit(2)
    if not entries:
        sys.stderr.write("Error: no search entry was repaired; has znai stopped doubling them?\n"
                         if not doc_id else
                         f"Error: no search entry carries /{doc_id}/; has znai stopped prefixing them?\n")
        sys.exit(2)
    if not pages:
        sys.stderr.write("Error: no page was pointed at its source; has znai started doing it?\n")
        sys.exit(2)
    print(f"repaired {stubs} redirect stubs, pointed {pages} pages at their source, "
          + (f"verified {links} internal links and {entries} search entries under /{doc_id}/, "
             f"pinned {refs} GitHub source links to {doc_id}"
             if doc_id else f"repaired {links} internal links and {entries} search entries"))


if __name__ == "__main__":
    main()
