#!/usr/bin/env python3
"""Stage the znai source tree, giving the site a section per root document.

Each section is named for the document it carries, so a reader can tell which file of the
repository they are reading; its pages keep the document's own titles. znai renders a directory
as a chapter and each file in it as a page, so a document either stays whole as a single page or
splits at its `##` boundaries. Section numbering stays in the source and never reaches the site:
a page carries the name alone, and the prose's `Section N` cross-references become links to the
pages they name. Repo-relative links, which znai would otherwise resolve as page references and
reject, become links to the sibling section or to the file on GitHub.
"""
from __future__ import annotations

import argparse
import json
import pathlib
import re
import shutil
import sys

REPO = "https://github.com/libfn/functional"

# The site's sections in order: a root document, the directory carrying it, and whether its `##`
# sections become pages of their own. README, CONTRIBUTING and LICENSE each read as one piece, so
# they stay whole; TYPE_ALGEBRA is long enough that its sections navigate better as pages. With no
# index.md of its own the site gets a generated one, redirecting the root to the first page below
# — so README needs no second copy to serve as a front page.
HAND_WRITTEN = None  # where the chapters docs/toc names are threaded into the order

SECTIONS = (
    ("README.md", "readme", False),
    ("TYPE_ALGEBRA.md", "type-algebra", True),
    ("CONTRIBUTING.md", "contributing", False),
    HAND_WRITTEN,
    ("LICENSE.md", "license", False),
)
DOCUMENTS = tuple(section for section in SECTIONS if section is not HAND_WRITTEN)

# A line that is nothing but a linked image: a badge, which belongs on the repository page and
# not on the documentation site. A bare image is content and stays.
BADGE = re.compile(r"^\[!\[[^\]]*\]\([^)]*\)\]\([^)]*\)$")

SUBTITLE = re.compile(r"^\*\*([^*]+)\*\*$")
NUMBERED = re.compile(r"^\d+\.\s+")
SECTION_REF = re.compile(r"\bSection (\d+)\b")
INLINE_LINK = re.compile(r"(?<=\])\((?!\()([^()\s]+)((?:\s+\"[^\"]*\")?)\)")
REFERENCE_LINK = re.compile(r"^(\[[^\]]+\]:\s*)(\S+)", re.MULTILINE)
HEADING = re.compile(r"^(#{3,})(?=\s)")
CODE_SPAN = re.compile(r"`+[^`]*`+")

# GitHub spells an admonition as a blockquote opening with an alert kind; znai spells it as a
# fenced attention block. znai builds a block only for the types below — `attention-tip` is not
# among them, and an unknown type falls through to a code snippet that prints the markdown
# verbatim, so an unmapped kind fails the staging instead of reaching the site looking like that.
ALERT = re.compile(r"^>\s*\[!([A-Z]+)\]\s*$")
ATTENTION = {"NOTE": "note", "TIP": "note", "WARNING": "warning"}
# Markers stand in for the block between the two rewrites, so that the alert's own body is
# rewritten as ordinary prose and only then fenced.
ATTENTION_OPEN = "<!--attention:{}-->"
ATTENTION_CLOSE = "<!--/attention-->"
MARKED = re.compile(r"^<!--attention:([a-z]+)-->$(.*?)^<!--/attention-->$",
                    re.MULTILINE | re.DOTALL)
# Unquoted openings that end a blockquote rather than continue its paragraph: a heading, a list
# item, a fence and a thematic break.
ENDS_QUOTE = re.compile(r"^(#{1,6}\s|[-*+]\s|\d+[.)]\s|```|~~~|(-{3,}|\*{3,}|_{3,})\s*$)")
FENCE = re.compile(r"^\s*(`{3,}|~{3,})")
BACKTICKS = re.compile(r"^\s*(`{3,})", re.MULTILINE)


def fail(message: str) -> None:
    sys.stderr.write(f"Error: {message}\n")
    sys.exit(2)


def slug(text: str) -> str:
    """The anchor GitHub derives from a heading, which is also the page name we give it."""
    return re.sub(r"[^a-z0-9]+", "-", text.lower()).strip("-")


def outside_fences(text: str):
    """Yield (index, line, inside_fence) so no rule fires on fenced content."""
    opening = None
    for index, line in enumerate(text.splitlines()):
        run = FENCE.match(line)
        # A fence closes on its own character and on a run no shorter than the one that opened it,
        # so a fence may quote a shorter one of either kind without ending itself.
        if run and (opening is None
                    or (run.group(1)[0] == opening[0] and len(run.group(1)) >= len(opening))):
            opening = None if opening else run.group(1)
            yield index, line, True
            continue
        yield index, line, opening is not None


def split(text: str, where: str) -> tuple[str, str, list[tuple[str, str]]]:
    """Split a document into its title, the prose above the first section, and the sections."""
    title = ""
    subtitle = True
    preamble: list[str] = []
    sections: list[tuple[str, list[str]]] = []

    for _, line, fenced in outside_fences(text):
        if not fenced:
            if not title and line.startswith("# "):
                title = line[2:].strip()
                continue
            # A document may follow its heading with a tagline, bold and alone on its line, which
            # completes the title rather than opening the prose.
            if title and subtitle and line.strip():
                match = SUBTITLE.match(line.strip())
                subtitle = False
                if match and not sections:
                    title = f"{title} - {match.group(1)}"
                    continue
            if line.startswith("## "):
                sections.append((line[3:].strip(), []))
                continue
        (sections[-1][1] if sections else preamble).append(line)

    if not title:
        fail(f"{where} has no level-one heading to title its chapter")
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
    for source, elsewhere, _split in DOCUMENTS:
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


def unquote_alerts(body: str) -> str:
    """Mark GitHub's alert blockquotes and strip the quoting from their bodies.

    The body is freed of its `>` here, ahead of the rewrites below, so that a heading or a link
    inside an alert is treated exactly like one outside it; the marker becomes a fence only once
    those rewrites have run, since a fenced body is left alone.
    """
    lines: list[str] = []
    alert = ALERT.match("")
    for _, line, fenced in outside_fences(body):
        if fenced:
            # A fence may quote alert syntax as an example of it; that is code, not an alert.
            if alert is not None:
                lines.append(ATTENTION_CLOSE)
                alert = None
            lines.append(line)
            continue
        if alert is None and (alert := ALERT.match(line)):
            kind = alert.group(1)
            if kind not in ATTENTION:
                fail(f"alert [!{kind}] has no znai attention block; "
                     f"known kinds are {', '.join(sorted(ATTENTION))}")
            lines.append(ATTENTION_OPEN.format(ATTENTION[kind]))
            continue
        if alert is not None:
            if line.startswith(">"):
                lines.append(line[2:] if line.startswith("> ") else line[1:])
                continue
            # A blockquote also swallows an unquoted line that merely continues its paragraph, and
            # only a block of its own ends it. Guessing which this is would silently split the
            # alert, so anything but a new block is refused and the author quotes it.
            if line.strip() and not ENDS_QUOTE.match(line):
                fail(f"alert body continues into an unquoted line: {line.strip()[:60]!r}; "
                     "prefix it with `>` or separate it with a blank line")
            lines.append(ATTENTION_CLOSE)
            alert = None
        lines.append(line)
    if alert is not None:
        lines.append(ATTENTION_CLOSE)
    return "\n".join(lines)


def fence_alerts(body: str) -> str:
    """Turn each marked alert into an attention block, fenced longer than anything inside it."""
    def block(match: re.Match[str]) -> str:
        inner = match.group(2).strip("\n")
        ticks = "`" * max(3, *(len(m.group(1)) + 1 for m in BACKTICKS.finditer(inner)), 3)
        return f"{ticks}attention-{match.group(1)}\n{inner}\n{ticks}"

    return MARKED.sub(block, body)


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
    for _, line, fenced in outside_fences(unquote_alerts(body)):
        if not fenced:
            if BADGE.match(line.strip()):
                continue
            if chapter is not None:
                # The section's own heading became the page title, so its subheadings move up to
                # take its place in znai's per-page navigation.
                line = HEADING.sub(lambda m: m.group(1)[1:], line)
            line = outside_code(line, prose)
        lines.append(line)
    return fence_alerts(re.sub(r"\n{3,}", "\n\n", "\n".join(lines)))


def write(path: pathlib.Path, title: str, body: str) -> None:
    path.write_text(f"---\ntitle: {json.dumps(title)}\n---\n\n{body}\n", encoding="utf-8")


def chapter(source: pathlib.Path, name: str, split_sections: bool,
            out: pathlib.Path, repo: pathlib.Path) -> list[str]:
    """Write one chapter and report its page names, in order, for the toc."""
    if not source.exists():
        fail(f"{source.name} does not exist")
    title, preamble, sections = split(source.read_text(encoding="utf-8"), source.name)
    if split_sections and not sections:
        fail(f"{source.name} has no level-two headings to split into pages")
    directory = out / name
    directory.mkdir(parents=True, exist_ok=True)

    if not split_sections:
        body = "\n\n".join([preamble] + [f"## {heading}\n\n{text}" for heading, text in sections])
        write(directory / "index.md", title, render(body, None, {}, None, repo))
        return ["index"]

    # Resolve every name a link may use before rendering any of it: both the anchor GitHub
    # would have produced for the original heading, and the number the prose refers to.
    page = {slug(heading): slug(NUMBERED.sub("", heading)) for heading, _ in sections}
    number = {NUMBERED.match(h).group(0).rstrip(". "): (NUMBERED.sub("", h), slug(NUMBERED.sub("", h)))
              for h, _ in sections if NUMBERED.match(h)}

    write(directory / "index.md", title, render(preamble, page, number, name, repo))
    names = ["index"]
    for heading, body in sections:
        stripped = NUMBERED.sub("", heading)
        write(directory / f"{slug(stripped)}.md", stripped, render(body, page, number, name, repo))
        names.append(slug(stripped))
    return names


def toc(out: pathlib.Path, generated: dict[str, list[str]]) -> None:
    """Rebuild the toc with the generated chapters ahead of the ones docs/toc names."""
    hand_written = (out / "toc").read_text(encoding="utf-8").strip().splitlines()
    lines = []
    for section in SECTIONS:
        if section is HAND_WRITTEN:
            lines += hand_written
            continue
        source, name, _split = section
        # A section is named for the document it carries, so a reader browsing the site can tell
        # which file of the repository they are reading; the pages keep the document's own titles.
        title = pathlib.Path(source).stem.replace("_", " ")
        lines += [f"{name} {{title: {json.dumps(title)}}}"]
        lines += [f"    {page}" for page in generated[name]]
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

    generated = {name: chapter(repo / source, name, split_sections, out, repo)
                 for source, name, split_sections in DOCUMENTS}
    toc(out, generated)


if __name__ == "__main__":
    main()
