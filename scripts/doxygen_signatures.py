#!/usr/bin/env python3
"""Keep the signature listings in docs/reference in step with the headers.

A reference page presents an overload set the way cppreference does: the whole set of
signatures in one listing, numbered, then one description per documented overload. znai
cannot draw that listing itself - its doxygen node carries no ref-qualifier, so the four
value-category overloads of a member render as four identical entries - so each listing is
written out in the page, titled with the member it covers, and checked here against the
doxygen XML.

    check          - every listing matches the overloads doxygen found
    emit <member>  - print a section body for a member, to paste into a page

`check` also reports descriptions that cannot reach the site: znai selects an overload by
its parameter types alone, so two documented overloads differing only by a requires-clause
are indistinguishable to it, and only the first is ever rendered.
"""

import argparse
import pathlib
import re
import sys
import xml.etree.ElementTree as ET
from collections import defaultdict

TITLED_FENCE = re.compile(
    r"^```cpp[ \t]*\{[^}]*\btitle[ \t]*:[ \t]*\"([^\"]+)\"[^}]*\}[ \t]*\n(.*?)^```[ \t]*$",
    re.M | re.S)
ANY_FENCE = re.compile(r"^```cpp\b", re.M)
NUMBER_COMMENT = re.compile(r"//\s*\(\d+\)\s*$")
TRAILING_CONSTRAINT = re.compile(r"\brequires\b.*$", re.S)


def prettify(text):
    """Spell types as a reader of the documentation would.

    Headers anchor the standard library as `::std::` so that a user's own `fn::std` cannot
    win lookup inside namespace `fn`; that hazard does not exist in prose, and doxygen's
    `Type< Arg >` spacing is its own, not the project's.
    """
    text = text.replace("::std::", "std::")
    text = re.sub(r"<\s+", "<", text)
    text = re.sub(r"\s+>", ">", text)
    return re.sub(r"\s+", " ", text).strip()


def normalize(line):
    """Compare listings by content, so a page is free to align its columns."""
    line = NUMBER_COMMENT.sub("", line)
    return re.sub(r"\s*([<>(),])\s*", r"\1", prettify(line)).rstrip(";").strip()


def xml_text(node):
    return " ".join("".join(node.itertext()).split()) if node is not None else ""


class Member:
    """One doxygen memberdef: an overload, and whether it carries documentation."""

    def __init__(self, node):
        self.node = node
        self.name = node.findtext("name")
        self.qualified = node.findtext("qualifiedname") or ""
        self.args = (node.findtext("argsstring") or "").strip()
        self.brief = xml_text(node.find("briefdescription"))
        self.detail = xml_text(node.find("detaileddescription"))
        types = ["".join(p.find("type").itertext())
                 for p in node.findall("param") if p.find("type") is not None]
        # znai compares parameter types with their spaces collapsed but the ones doxygen
        # puts inside angle brackets kept, so a selector must be spelled its way.
        self.selector = ",".join(t.replace(" &", "&").replace(" *", "*") for t in types)
        self.args_text = ", ".join(types)

    @property
    def documented(self):
        return bool(self.brief or self.detail)

    def describes(self, *kinds):
        """Whether the description feeds znai an api-parameters table of this kind."""
        detail = self.node.find("detaileddescription")
        if detail is None:
            return False
        found = {p.get("kind") for p in detail.iter("parameterlist")}
        found |= {s.get("kind") for s in detail.iter("simplesect")}
        return bool(found & set(kinds))

    @property
    def declared_type(self):
        """doxygen's <type>, less what belongs elsewhere in the declaration.

        A trailing requires-clause lands here, and a hidden friend carries its `friend`
        here rather than among the attributes.
        """
        declared = xml_text(self.node.find("type"))
        if declared.startswith("friend "):
            declared = declared[len("friend "):]
        return prettify(TRAILING_CONSTRAINT.sub("", declared))

    @property
    def returns(self):
        """The trailing return type to draw, if any.

        A deduced `auto` says nothing a reader can use, and an alias carries its type in
        the declaration itself, so neither draws an arrow.
        """
        if self.declared_type == "auto" or self.node.get("kind") == "typedef":
            return ""
        return self.declared_type

    @property
    def template_line(self):
        params = self.node.find("templateparamlist")
        if params is None:
            return None
        spelled = [f"{xml_text(p.find('type'))} {xml_text(p.find('declname'))}".strip()
                   for p in params]
        return prettify(f"template <{', '.join(spelled)}>")

    def declaration(self):
        """As the headers declare it: trailing return type, no noexcept, no constraint.

        doxygen glues a plain `noexcept` onto argsstring and omits a conditional one; both
        are left out for the reason cppreference leaves them out - a listing is for
        scanning, and the conditions belong in the prose.
        """
        if self.node.get("kind") == "typedef":
            return prettify(f"using {self.name} = {self.declared_type}")
        if not xml_text(self.node.find("type")):  # a guide keeps its arrow in argsstring
            return prettify(f"{self.name}{self.args}")
        args = re.sub(r"\s*\bnoexcept\b\s*$", "", re.sub(r"\s*->.*$", "", self.args)).strip()
        keywords = [k for k, v in (("static", self.node.get("static")),
                                   ("explicit", self.node.get("explicit")),
                                   ("constexpr", self.node.get("constexpr")))
                    if v == "yes"]
        if xml_text(self.node.find("type")).startswith("friend "):
            keywords.insert(0, "friend")
        return prettify(f"{' '.join(keywords + ['auto'])} {self.name}{args}")


def load_members(xml_dir):
    members = defaultdict(list)
    for path in sorted(pathlib.Path(xml_dir).glob("*.xml")):
        if path.name == "index.xml":
            continue
        for compound in ET.parse(path).getroot().iter("compounddef"):
            cname = compound.findtext("compoundname") or ""
            for node in compound.iter("memberdef"):
                member = Member(node)
                members[member.qualified or f"{cname}::{member.name}"].append(member)
    return members


def lookup(name, members):
    """Pages name a class template as the headers do; doxygen strips the arguments."""
    if name in members:
        return members[name]
    stripped = re.sub(r"<[^<>]*>", "", name).replace(" ", "")
    for key, value in members.items():
        if key.replace(" ", "") == stripped:
            return value
    return None


def distinct(overloads):
    """Overloads a listing can tell apart.

    Two overloads separated only by a requires-clause declare identically, so they earn one
    line between them rather than a run of repeats the reader cannot account for.
    """
    out = []
    for member in overloads:
        shape = (member.template_line, member.declaration(), member.returns)
        if not out or out[-1][0] != shape:
            out.append((shape, member))
    return [member for _, member in out]


def listing(overloads):
    """Signatures for one member, grouped by template line and aligned for reading."""
    shown = distinct(overloads)
    groups, current = [], []
    for i, member in enumerate(shown):
        if current and member.template_line != shown[i - 1].template_line:
            groups.append(current)
            current = []
        current.append((i + 1, member))
    groups.append(current)

    out = []
    for group in groups:
        if out:
            out.append("")
        if group[0][1].template_line:
            out.append(group[0][1].template_line)
        decls = [m.declaration() for _, m in group]
        arrows = [m.returns for _, m in group]
        width = max(len(d) for d in decls)
        span = max((len(r) for r in arrows if r), default=0)
        for (number, _), decl, ret in zip(group, decls, arrows):
            line = f"{decl:<{width}} -> {ret};" if ret else f"{decl};"
            pad = width + len(" -> ") + span + 1 if span else width + 1
            out.append(f"{line:<{pad}}  // ({number})")
    return "\n".join(line.rstrip() for line in out)


def selectors(overloads):
    """The distinct parameter-type lists znai can select an overload by, in order."""
    groups = {}
    for member in overloads:
        groups.setdefault(member.selector, []).append(member)
    return groups


def check(pages, members):
    problems = []
    for page in pages:
        text = page.read_text()
        titled = TITLED_FENCE.findall(text)
        if len(ANY_FENCE.findall(text)) != len(titled):
            problems.append(f"{page.name}: a cpp listing has no {{title: \"<member>\"}}")
        for name, body in titled:
            overloads = lookup(name, members)
            if overloads is None:
                problems.append(f"{page.name}: {name} is not in the doxygen XML")
                continue
            want = [normalize(x) for x in listing(overloads).splitlines() if x.strip()]
            have = [normalize(x) for x in body.splitlines() if x.strip()]
            if want == have:
                continue
            problems.append(f"{page.name}: the listing for {name} does not match the headers")
            problems += [f"    missing: {x}" for x in want if x not in have]
            problems += [f"    stale:   {x}" for x in have if x not in want]
    return problems


def unreachable(members_by_page):
    problems = []
    for page, name, overloads in members_by_page:
        for selector, group in selectors(overloads).items():
            documented = [m for m in group if m.documented]
            if len(documented) > 1:
                problems.append(
                    f"{page}: {name} has {len(documented)} descriptions sharing the "
                    f"selector {selector!r}; znai can render only the first")
    return problems


def section(name, overloads):
    """A whole section body: the listing, then what znai can select per overload."""
    out = [f'```cpp {{title: "{name}"}}', listing(overloads), "```"]
    for group in selectors(overloads).values():
        first = group[0]
        if not first.documented:
            continue
        out.append("")
        out.append(f':include-doxygen-doc: {name} {{ args: "{first.args_text}" }}')
        # the standalone params plugin titles nothing of its own, and two untitled tables
        # in a row read as one
        if first.describes("templateparam"):
            out.append("")
            out.append(f':include-doxygen-doc-params: {name} {{ args: "{first.args_text}", '
                       f'type: "template", title: "template parameters" }}')
        if first.describes("param", "return"):
            out.append("")
            out.append(f':include-doxygen-doc-params: {name} {{ args: "{first.args_text}", '
                       f'title: "parameters" }}')
    return "\n".join(out)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("command", choices=("check", "emit"))
    parser.add_argument("names", nargs="*", help="members to emit a section for")
    parser.add_argument("--xml", required=True, help="doxygen XML output directory")
    parser.add_argument("--docs", required=True, help="the docs/reference directory")
    args = parser.parse_args()

    members = load_members(args.xml)
    pages = sorted(pathlib.Path(args.docs).glob("*.md"))

    if args.command == "emit":
        for name in args.names:
            overloads = lookup(name, members)
            if overloads is None:
                raise SystemExit(f"{name} is not in the doxygen XML")
            print(section(name, overloads))
        return 0

    named = [(page.name, name, lookup(name, members))
             for page in pages
             for name, _ in TITLED_FENCE.findall(page.read_text())]
    problems = check(pages, members) + unreachable(
        [(p, n, o) for p, n, o in named if o is not None])
    for problem in problems:
        print(problem, file=sys.stderr)
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
