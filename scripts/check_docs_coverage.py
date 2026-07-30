#!/usr/bin/env python3
"""Check that what is documented reaches the documentation.

Three ways a description silently fails to arrive, each of which has happened here:

  unpublished   an entity carries a doxygen stanza that no page names, so the words exist and
                the site never shows them - forty-eight carrier members were in this state
  empty         a page asks for a compound that carries no description, and znai renders
                silence rather than complaining - fn::choice's page opened with nothing
  unrendered    a page asks for an overload set whose first member is the undocumented one,
                and znai renders the first member's description, which is empty - operator&
                and operator| were in this state

Entities deliberately left undocumented are listed in the exemption file, with a reason, so
that the omission is a decision on the record and anything new fails instead.
"""

import argparse
import pathlib
import re
import sys
import xml.etree.ElementTree as ET

# the library marks what is not its interface with a leading underscore, and keeps its
# implementation in `detail`
INTERNAL = re.compile(r"(^|::)(detail|_impl)(::|$)|(^|::)_")
DOC_DIRECTIVE = re.compile(r"^:include-doxygen-doc:[ \t]+([^{\n]+?)(?:[ \t]*\{(.*)\})?[ \t]*$",
                           re.M)
ARGS_OPT = re.compile(r"""\bargs\s*:\s*"([^"]*)\"""")


def described(node):
    for tag in ("briefdescription", "detaileddescription"):
        child = node.find(tag)
        if child is not None and "".join(child.itertext()).strip():
            return True
    return False


def user_facing(name):
    return bool(name) and name.split("::")[0] in ("fn", "pfn") and not INTERNAL.search(name)


class Api:
    """Every user-facing entity doxygen found, and how it is documented."""

    def __init__(self, xml_dir):
        self.compounds = {}                  # name -> described
        self.members = {}                    # name -> [(selector, described)] in doxygen order
        for path in sorted(pathlib.Path(xml_dir).glob("*.xml")):
            if path.name == "index.xml":
                continue
            for compound in ET.parse(path).getroot().iter("compounddef"):
                cname = compound.findtext("compoundname") or ""
                if compound.get("kind") not in ("file", "dir") and user_facing(cname):
                    self.compounds[cname] = described(compound)
                for node in compound.iter("memberdef"):
                    if node.get("prot") not in (None, "public") or node.get("kind") == "friend":
                        continue
                    qualified = node.findtext("qualifiedname") or ""
                    if not user_facing(qualified):
                        continue
                    types = ["".join(p.find("type").itertext()).replace(" &", "&")
                             for p in node.findall("param") if p.find("type") is not None]
                    self.members.setdefault(qualified, []).append(
                        (",".join(types), described(node)))

    def documented(self):
        for name, flag in self.compounds.items():
            if flag:
                yield name
        for name, overloads in self.members.items():
            if any(flag for _, flag in overloads):
                yield name


def load_exemptions(path):
    allowed = {}
    if not path.is_file():
        return allowed
    for number, line in enumerate(path.read_text().splitlines(), 1):
        line = line.split("#", 1)[0].strip() if not line.lstrip().startswith("#") else ""
        if not line:
            continue
        # a colon FOLLOWED BY A SPACE separates the two: a qualified name is full of `::`
        if ": " not in line:
            raise SystemExit(f"{path}:{number}: expected '<entity>: <reason>'")
        name, reason = line.split(": ", 1)
        if not reason.strip():
            raise SystemExit(f"{path}:{number}: {name.strip()} needs a reason")
        allowed[name.strip()] = reason.strip()
    return allowed


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--xml", required=True, help="doxygen XML output directory")
    parser.add_argument("--docs", required=True, help="the docs/ directory")
    args = parser.parse_args()

    api = Api(args.xml)
    docs = pathlib.Path(args.docs)
    pages = sorted(docs.rglob("*.md"))
    text = " ".join(p.read_text() for p in pages)
    exempt = load_exemptions(docs / "coverage-exemptions.txt")

    problems, used = [], set()
    for name in sorted(api.documented()):
        if name in text:
            continue
        if name in exempt:
            used.add(name)
            continue
        problems.append(f"{name} is documented and no page names it")

    for page in pages:
        for name, opts in DOC_DIRECTIVE.findall(page.read_text()):
            name = name.strip()
            if name in api.compounds:
                if not api.compounds[name]:
                    problems.append(f"{page.name}: {name} carries no description, so the page "
                                    f"renders nothing there")
                continue
            overloads = api.members.get(name)
            if overloads is None:
                continue
            wanted = ARGS_OPT.search(opts or "")
            selector = None
            if wanted:
                query = re.sub(r",\s+", ",", wanted.group(1).strip())
                selector = re.sub(r"\s+", " ", query).replace(" &", "&").replace(" *", "*")
            first = next(((s, d) for s, d in overloads if selector is None or s == selector), None)
            if first and not first[1]:
                problems.append(f"{page.name}: {name} renders the description of an overload "
                                f"that has none - the documented one is not the first")

    for name in sorted(set(exempt) - used):
        problems.append(f"{name} is exempted but no longer needs to be; drop it from "
                        f"coverage-exemptions.txt")

    for problem in problems:
        print(problem, file=sys.stderr)
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
