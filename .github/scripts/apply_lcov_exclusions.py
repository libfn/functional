#!/usr/bin/env python3
r"""Apply LCOV line exclusions to gcov text files in place for SonarCloud.

The markers remain source text in gcov output. Rewrite excluded lines as non-executable (`-`)
and remove their branch, call and unconditional-branch records. `LCOV_EXCL_LINE` excludes its
line; `LCOV_EXCL_START` begins an excluded range that ends before the `LCOV_EXCL_STOP` line.
Branch-only markers (`LCOV_EXCL_BR_*`) are not handled.

Usage: apply_lcov_exclusions.py <directory>

gcov can repeat source lines for template instantiations. Determine exclusions in source-line
order, then apply them to every occurrence of each line, including repeated blocks that omit
the range markers.
"""

import os
import pathlib
import re
import stat
import sys
import tempfile

RECORD = re.compile(r"^ *(?P<count>[^ :]+): *(?P<line>[0-9]+):(?P<source>.*)$")
ANNOTATION = re.compile(r"^(branch|call|unconditional) ")
NO_CODE = "        -:"


def excluded_lines(lines):
    source = {}
    for line in lines:
        record = RECORD.match(line)
        if record and int(record["line"]) > 0:
            source.setdefault(int(record["line"]), record["source"])
    excluded, in_range = set(), False
    for number in sorted(source):
        text = source[number]
        if "LCOV_EXCL_STOP" in text:
            in_range = False
        if "LCOV_EXCL_START" in text:
            in_range = True
        if in_range or "LCOV_EXCL_LINE" in text:
            excluded.add(number)
    return excluded


def apply(lines, excluded):
    r"""Rewrite excluded source records and omit their coverage annotations.

    >>> sample = '''\
    ...         -:    0:Source:LCOV_EXCL_START.cpp
    ...         1:    1:int f() {
    ...     #####:    2:  unreachable(); // LCOV_EXCL_LINE
    ... call    0 never executed
    ...        1*:    3:  x(); // LCOV_EXCL_START
    ...     =====:    4:  y();
    ... branch  0 never executed
    ...         1:    5:  z(); // LCOV_EXCL_STOP
    ... branch  0 taken 1
    ...         1:    6:  w(); // LCOV_EXCL_BR_LINE
    ... ------------------
    ... _Z1fIiEvv:
    ...     #####:    2:  unreachable(); // LCOV_EXCL_LINE
    ... call    0 never executed
    ...     =====:    4:  y();
    ... '''.splitlines(keepends=True)
    >>> print("".join(apply(sample, excluded_lines(sample))), end="")
            -:    0:Source:LCOV_EXCL_START.cpp
            1:    1:int f() {
            -:    2:  unreachable(); // LCOV_EXCL_LINE
            -:    3:  x(); // LCOV_EXCL_START
            -:    4:  y();
            1:    5:  z(); // LCOV_EXCL_STOP
    branch  0 taken 1
            1:    6:  w(); // LCOV_EXCL_BR_LINE
    ------------------
    _Z1fIiEvv:
            -:    2:  unreachable(); // LCOV_EXCL_LINE
            -:    4:  y();
    """
    drop = False
    for line in lines:
        record = RECORD.match(line)
        if record:
            drop = int(record["line"]) in excluded
            if drop:
                line = NO_CODE + line[record.end("count") + 1:]
        elif drop and ANNOTATION.match(line):
            continue
        yield line


def gcov_files(root):
    """Yield regular .gcov files below root, skipping symlinks and raising traversal errors."""

    def fail(error):
        raise error

    for dirpath, _, names in os.walk(root, onerror=fail):
        for name in sorted(names):
            path = pathlib.Path(dirpath, name)
            if name.endswith(".gcov") and stat.S_ISREG(os.lstat(path).st_mode):
                yield path


def main(argv):
    if len(argv) != 2:
        sys.exit(f"usage: {argv[0]} <directory>")
    root = pathlib.Path(argv[1])
    if not root.is_dir():
        sys.exit(f"{root}: not a directory")
    # Only a line feed ends a record: a stray carriage return inside a source line stays inside it.
    text = dict(encoding="utf-8", errors="surrogateescape", newline="\n")
    for path in gcov_files(root):
        with open(path, **text) as f:
            excluded = excluded_lines(f)
        fd, tmp = tempfile.mkstemp(prefix=path.name + ".", dir=path.parent)
        with open(path, **text) as src, os.fdopen(fd, "w", **text) as dst:
            dst.writelines(apply(src, excluded))
        os.replace(tmp, path)


if __name__ == "__main__":
    main(sys.argv)
