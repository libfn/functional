#!/usr/bin/env python3
"""Stage the znai source tree, giving the site a front page and a chapter per root document.

README is the front page, whole: it introduces the library once, and splitting it would scatter
that introduction. The longer documents become chapters instead — znai renders a directory as a
chapter and each file in it as a page, so they reach the site split at their `##` boundaries.
Section numbering stays in the source and never reaches the site: page titles carry the name
alone, and the prose's `Section N` cross-references become links to the pages they name.
Repo-relative links, which znai would otherwise resolve as page references and reject, become
links to the sibling chapter or to the file on GitHub.
"""
from __future__ import annotations

import argparse
import json
import pathlib
import re
import shutil
import sys

REPO = "https://github.com/libfn/functional"

LANDING = "README.md"

# Root document -> chapter directory. The toc lists LEADING, then the chapters docs/toc names,
# then TRAILING; znai titles a chapter from its directory, so `type-algebra` reads "Type Algebra".
LEADING = (("TYPE_ALGEBRA.md", "type-algebra"),)
TRAILING = (("CONTRIBUTING.md", "contributing"),)

NUMBERED = re.compile(r"^\d+\.\s+")
SECTION_REF = re.compile(r"\bSection (\d+)\b")
INLINE_LINK = re.compile(r"(?<=\])\((?!\()([^()\s]+)((?:\s+\"[^\"]*\")?)\)")
REFERENCE_LINK = re.compile(r"^(\[[^\]]+\]:\s*)(\S+)", re.MULTILINE)
HEADING = re.compile(r"^(#{3,})(?=\s)")
CODE_SPAN = re.compile(r"`+[^`]*`+")


def fail(message: str) -> None:
    sys.stderr.write(f"Error: {message}\n")
    sys.exit(2)


def slug(text: str) -> str:
    """The anchor GitHub derives from a heading, which is also the page name we give it."""
    return re.sub(r"[^a-z0-9]+", "-", text.lower()).strip("-")


def outside_fences(text: str):
    """Yield (index, line, inside_fence) so no rule fires on fenced content."""
    fence = False
    for index, line in enumerate(text.splitlines()):
        if line.lstrip().startswith("```"):
            fence = not fence
            yield index, line, True
            continue
        yield index, line, fence


def split(text: str, where: str) -> tuple[str, str, list[tuple[str, str]]]:
    """Split a document into its title, the prose above the first section, and the sections."""
    title = ""
    preamble: list[str] = []
    sections: list[tuple[str, list[str]]] = []

    for _, line, fenced in outside_fences(text):
        if not fenced:
            if not title and line.startswith("# "):
                title = line[2:].strip()
                continue
            if line.startswith("## "):
                sections.append((line[3:].strip(), []))
                continue
        (sections[-1][1] if sections else preamble).append(line)

    if not title:
        fail(f"{where} has no level-one heading to title its chapter")
    if not sections:
        fail(f"{where} has no level-two headings to split into pages")
    return title, "\n".join(preamble).strip(), [(t, "\n".join(b).strip()) for t, b in sections]


def target(link: str, page: dict[str, str] | None, chapter: str, repo: pathlib.Path) -> str:
    """Point a document-relative link at the staged site, or at the file on GitHub."""
    if re.match(r"^[a-z][a-z0-9+.-]*:", link) or link.startswith("//"):
        return link
    if link.startswith("#"):
        # The front page keeps its own anchors; a chapter has none, its sections being pages.
        if page is None:
            return link
        anchor = link[1:]
        if anchor not in page:
            fail(f"link to #{anchor} matches no section")
        return f"{chapter}/{page[anchor]}"

    path, _, anchor = link.partition("#")
    if path == LANDING:
        return "/"
    for source, elsewhere in LEADING + TRAILING:
        if path == source:
            return f"{elsewhere}/index"
    if not (repo / path).exists():
        fail(f"link to {path} matches no file in the repository")
    kind = "tree" if (repo / path).is_dir() else "blob"
    return f"{REPO}/{kind}/main/{path.rstrip('/')}" + (f"#{anchor}" if anchor else "")


def outside_code(line: str, rewrite) -> str:
    """Apply a rewrite to the prose of a line, leaving its inline code alone.

    A lambda in inline code (`[](auto)`) is otherwise indistinguishable from a markdown link.
    """
    parts, last = [], 0
    for span in CODE_SPAN.finditer(line):
        parts += [rewrite(line[last:span.start()]), span.group(0)]
        last = span.end()
    return "".join(parts) + rewrite(line[last:])


def render(body: str, page: dict[str, str] | None, number: dict[str, tuple[str, str]],
           chapter: str | None, repo: pathlib.Path) -> str:
    """Rewrite a body for the site: heading depth, links, cross-references."""
    def prose(text: str) -> str:
        text = INLINE_LINK.sub(
            lambda m: f"({target(m.group(1), page, chapter, repo)}{m.group(2)})", text)
        text = REFERENCE_LINK.sub(
            lambda m: m.group(1) + target(m.group(2), page, chapter, repo), text)
        return SECTION_REF.sub(
            lambda m: f"[{number[m.group(1)][0]}]({chapter}/{number[m.group(1)][1]})"
            if m.group(1) in number
            else fail(f"reference to Section {m.group(1)}, which {chapter} does not have"), text)

    lines = []
    for _, line, fenced in outside_fences(body):
        if not fenced:
            if chapter is not None:
                # The section's own heading became the page title, so its subheadings move up to
                # take its place in znai's per-page navigation.
                line = HEADING.sub(lambda m: m.group(1)[1:], line)
            line = outside_code(line, prose)
        lines.append(line)
    return "\n".join(lines)


def landing(source: pathlib.Path, out: pathlib.Path, repo: pathlib.Path) -> None:
    """Write the front page: the document entire, its headings and anchors left as they are."""
    if not source.exists():
        fail(f"{source.name} does not exist")
    body = render(source.read_text(encoding="utf-8"), None, {}, None, repo)
    (out / "index.md").write_text(body.strip() + "\n", encoding="utf-8")


def write(path: pathlib.Path, title: str, body: str) -> None:
    path.write_text(f"---\ntitle: {json.dumps(title)}\n---\n\n{body}\n", encoding="utf-8")


def chapter(source: pathlib.Path, name: str, out: pathlib.Path, repo: pathlib.Path) -> list[str]:
    """Write one chapter and report the page names, in order, for the toc."""
    if not source.exists():
        fail(f"{source.name} does not exist")
    title, preamble, sections = split(source.read_text(encoding="utf-8"), source.name)

    # Resolve every name a link may use before rendering any of it: both the anchor GitHub
    # would have produced for the original heading, and the number the prose refers to.
    page = {slug(heading): slug(NUMBERED.sub("", heading)) for heading, _ in sections}
    number = {NUMBERED.match(h).group(0).rstrip(". "): (NUMBERED.sub("", h), slug(NUMBERED.sub("", h)))
              for h, _ in sections if NUMBERED.match(h)}

    directory = out / name
    directory.mkdir(parents=True, exist_ok=True)
    write(directory / "index.md", title, render(preamble, page, number, name, repo))

    names = ["index"]
    for heading, body in sections:
        stripped = NUMBERED.sub("", heading)
        write(directory / f"{slug(stripped)}.md", stripped, render(body, page, number, name, repo))
        names.append(slug(stripped))
    return names


def toc(out: pathlib.Path, generated: dict[str, list[str]]) -> None:
    """Rebuild the toc with the generated chapters around the ones docs/toc names."""
    lines = []
    for _, name in LEADING:
        lines += [name] + [f"    {page}" for page in generated[name]]
    lines += (out / "toc").read_text(encoding="utf-8").strip().splitlines()
    for _, name in TRAILING:
        lines += [name] + [f"    {page}" for page in generated[name]]
    (out / "toc").write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("repo", type=pathlib.Path, help="repository root")
    parser.add_argument("out", type=pathlib.Path, help="staged znai source directory")
    args = parser.parse_args()

    repo, out = args.repo.resolve(), args.out.resolve()
    if out.exists():
        shutil.rmtree(out)
    shutil.copytree(repo / "docs", out)

    # docs/lookup-paths reaches the sources it quotes relatively, which the staged copy is no
    # longer placed to do.
    paths = (repo / "docs" / "lookup-paths").read_text(encoding="utf-8").split()
    (out / "lookup-paths").write_text(
        "".join(f"{(repo / 'docs' / path).resolve()}\n" for path in paths), encoding="utf-8")

    landing(repo / LANDING, out, repo)
    generated = {name: chapter(repo / source, name, out, repo)
                 for source, name in LEADING + TRAILING}
    toc(out, generated)


if __name__ == "__main__":
    main()
