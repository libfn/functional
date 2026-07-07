#!/usr/bin/env python3
"""Keep the README.md code example in sync with examples/readme/main.cpp.

examples/readme/main.cpp is the single source of truth: CI builds and runs it,
so the code README.md shows is proven to compile and pass its own checks. The
region between its two `// readme-example` marker lines is mirrored into the
```cpp fence under README.md's "## Example" heading. Run as a pre-commit hook:
it rewrites the fence and exits non-zero if it changed anything, so the commit
is blocked until the change is re-staged.
"""
import pathlib
import sys

MARKER = "// readme-example"
HEADING = "## Example"

repo = pathlib.Path(__file__).resolve().parents[1]
example_path = repo / "examples" / "readme" / "main.cpp"
readme_path = repo / "README.md"

lines = example_path.read_text().splitlines(keepends=True)
marks = [i for i, line in enumerate(lines) if line.rstrip("\n") == MARKER]
if len(marks) != 2:
    sys.stderr.write(
        f"{example_path.relative_to(repo)}: expected exactly two {MARKER!r} lines, found {len(marks)}\n"
    )
    sys.exit(1)
region = lines[marks[0] + 1 : marks[1]]

readme = readme_path.read_text().splitlines(keepends=True)
try:
    heading = next(i for i, line in enumerate(readme) if line.rstrip("\n") == HEADING)
    fence = next(i for i in range(heading + 1, len(readme)) if readme[i].rstrip("\n") == "```cpp")
    close = next(i for i in range(fence + 1, len(readme)) if readme[i].rstrip("\n") == "```")
except StopIteration:
    sys.stderr.write(
        f"{readme_path.relative_to(repo)}: could not find {HEADING!r} followed by a ```cpp fence\n"
    )
    sys.exit(1)

if readme[fence + 1 : close] != region:
    readme[fence + 1 : close] = region
    readme_path.write_text("".join(readme))
    print(f"synced {readme_path.relative_to(repo)} example <- {example_path.relative_to(repo)}")
    sys.exit(1)
sys.exit(0)
