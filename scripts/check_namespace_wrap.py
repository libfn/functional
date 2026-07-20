#!/usr/bin/env python3
"""Check that every fn/pfn namespace in include/ carries inline namespace LIBFN_VERSION.

An unwrapped opening silently lands its entities outside the versioned ABI
namespace: every source-tree build stays green, while the entities' mangled
names escape the version and break ABI coexistence of different library
versions in one program. Run as a pre-commit hook: exits non-zero naming each
unwrapped opening as file:line.
"""
import pathlib
import sys

repo = pathlib.Path(__file__).resolve().parents[1]
include = repo / "include"

errors = []
for path in sorted(include.rglob("*.hpp")):
    lines = path.read_text().splitlines()
    for lineno, line in enumerate(lines, start=1):
        if line in ("namespace fn {", "namespace pfn {"):
            if lineno >= len(lines) or lines[lineno] != "inline namespace LIBFN_VERSION {":
                errors.append((path, lineno, line))
        elif line.startswith(("namespace fn::", "namespace pfn::")):
            head = line.split("::", 1)[0]
            if not line.startswith(head + "::inline LIBFN_VERSION::"):
                errors.append((path, lineno, line))

for path, lineno, line in errors:
    print(
        f"{path.relative_to(repo).as_posix()}:{lineno}: '{line}' does not open inline namespace"
        " LIBFN_VERSION: its entities would land outside the versioned ABI namespace"
    )
sys.exit(1 if errors else 0)
