#!/usr/bin/env python3
"""Check that every header under include/ is listed in include/CMakeLists.txt.

The install file sets (INCLUDE_PFN_HEADERS / INCLUDE_FN_HEADERS) enumerate files
explicitly. A header missing from the list is silently absent from the installed
package: every source-tree build stays green (they use the include/ directory
wholesale), while the package-test CI lanes, which compile a consumer against the
installed tree, fail with "No such file or directory". Run as a pre-commit hook:
exits non-zero naming any header on disk that is not listed, or any listed header
that does not exist on disk.
"""
import pathlib
import re
import sys

repo = pathlib.Path(__file__).resolve().parents[1]
include = repo / "include"
cmakelists = include / "CMakeLists.txt"

listed = set(re.findall(r"^\s+((?:pfn|fn)/\S+\.hpp)\s*$", cmakelists.read_text(), re.MULTILINE))
on_disk = {p.relative_to(include).as_posix() for p in include.rglob("*.hpp")}

missing = sorted(on_disk - listed)
stale = sorted(listed - on_disk)
for f in missing:
    print(
        f"include/{f} is not listed in include/CMakeLists.txt: it would be missing from"
        " the installed package and fail the package-test CI lanes"
    )
for f in stale:
    print(f"include/CMakeLists.txt lists {f}, which does not exist on disk")
sys.exit(1 if missing or stale else 0)
