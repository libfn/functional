#!/usr/bin/env python3
"""Keep markdown document code examples in sync with their verified C++ source files."""
import argparse
import pathlib
import re
import sys

def sync_readme(repo: pathlib.Path) -> bool:
    MARKER = "// readme-example"
    HEADING = "## Example"

    example_path = repo / "examples" / "readme" / "main.cpp"
    readme_path = repo / "README.md"

    if not example_path.exists():
        sys.stderr.write(f"Error: {example_path} does not exist\n")
        sys.exit(2)
    if not readme_path.exists():
        sys.stderr.write(f"Error: {readme_path} does not exist\n")
        sys.exit(2)

    lines = example_path.read_text(encoding="utf-8").splitlines(keepends=True)
    marks = [i for i, line in enumerate(lines) if line.rstrip("\r\n") == MARKER]
    if len(marks) != 2:
        sys.stderr.write(
            f"{example_path.relative_to(repo)}: expected exactly two {MARKER!r} lines, found {len(marks)}\n"
        )
        sys.exit(2)
    region = lines[marks[0] + 1 : marks[1]]

    readme = readme_path.read_text(encoding="utf-8").splitlines(keepends=True)
    try:
        heading = next(i for i, line in enumerate(readme) if line.rstrip("\r\n") == HEADING)
        fence = next(i for i in range(heading + 1, len(readme)) if readme[i].rstrip("\r\n") == "```cpp")
        close = next(i for i in range(fence + 1, len(readme)) if readme[i].rstrip("\r\n") == "```")
    except StopIteration:
        sys.stderr.write(
            f"{readme_path.relative_to(repo)}: could not find {HEADING!r} followed by a ```cpp fence\n"
        )
        sys.exit(2)

    if readme[fence + 1 : close] != region:
        readme[fence + 1 : close] = region
        readme_path.write_text("".join(readme), encoding="utf-8")
        print(f"synced {readme_path.relative_to(repo)} example <- {example_path.relative_to(repo)}")
        return True # changed
    return False # no change

def sync_type_algebra(repo: pathlib.Path) -> bool:
    example_path = repo / "examples" / "type_algebra" / "main.cpp"
    doc_path = repo / "TYPE_ALGEBRA.md"

    if not example_path.exists():
        sys.stderr.write(f"Error: {example_path} does not exist\n")
        sys.exit(2)
    if not doc_path.exists():
        sys.stderr.write(f"Error: {doc_path} does not exist\n")
        sys.exit(2)

    # 1. Parse all regions from examples/type_algebra/main.cpp
    example_text = example_path.read_text(encoding="utf-8")
    example_lines = example_text.splitlines()

    regions = {}
    current_name = None
    current_block = []

    for line in example_lines:
        stripped = line.strip()
        if stripped.startswith("// sync-example-"):
            name = stripped[len("// sync-example-"):]
            if current_name is None:
                # Start of a region
                current_name = name
                current_block = []
            elif current_name == name:
                # End of a region
                regions[current_name] = "\n".join(current_block)
                current_name = None
            else:
                sys.stderr.write(f"Error: unmatched boundary in main.cpp: started {current_name}, got {name}\n")
                sys.exit(2)
        elif current_name is not None:
            current_block.append(line)

    print(f"Extracted {len(regions)} verified code regions from {example_path.name}")

    # 2. Read and synchronize TYPE_ALGEBRA.md
    doc_text = doc_path.read_text(encoding="utf-8")

    # We find <!-- sync-example-<name> --> optionally prefixed by blockquote indicators like '> '
    # followed by ```cpp <content> ```, and replace it while preserving the prefix on every line.
    def replace_snippet(match):
        prefix = match.group(1)
        name = match.group(2)
        if name not in regions:
            sys.stderr.write(f"Warning: no matching region in main.cpp for sync-example-{name}\n")
            return match.group(0) # Keep unchanged

        # Prefix each line of the synchronized example block if we are inside a blockquote
        region_lines = regions[name].splitlines()
        if prefix:
            prefixed_region = "\n".join(f"{prefix}{line}" if line.strip() else prefix.rstrip() for line in region_lines)
        else:
            prefixed_region = "\n".join(region_lines)

        return f"{prefix}<!-- sync-example-{name} -->\n{prefix}```cpp\n{prefixed_region}\n{prefix}```"

    # Match: optional blockquote prefix (e.g., '> '), comment, opening fence, body, and closing fence
    pattern = re.compile(
        r"^([ >]*?)<!-- sync-example-([\w-]+) -->\s*?\n[ >]*?```cpp\n(.*?)\n[ >]*?```",
        re.MULTILINE | re.DOTALL
    )
    updated_text = pattern.sub(replace_snippet, doc_text)

    # Clean up trailing whitespace in the document (like pre-commit trailing whitespace check does)
    updated_text = re.sub(r"[ \t]+$", "", updated_text, flags=re.MULTILINE)

    if doc_text != updated_text:
        doc_path.write_text(updated_text, encoding="utf-8")
        print(f"Synced code fences in {doc_path.relative_to(repo)} <- {example_path.relative_to(repo)}")
        return True # changed
    return False # no change

def main() -> None:
    parser = argparse.ArgumentParser(description="Synchronize markdown files with verified C++ examples.")
    parser.add_argument("mode", choices=["readme", "type-algebra"], help="The markdown file/examples to sync.")
    args = parser.parse_args()

    repo = pathlib.Path(__file__).resolve().parents[1]

    if args.mode == "readme":
        changed = sync_readme(repo)
    elif args.mode == "type-algebra":
        changed = sync_type_algebra(repo)
    else:
        sys.stderr.write(f"Unknown mode: {args.mode}\n")
        sys.exit(2)

    if changed:
        sys.exit(1)

    print(f"All {args.mode} code examples are fully synchronized!")
    sys.exit(0)

if __name__ == "__main__":
    main()
