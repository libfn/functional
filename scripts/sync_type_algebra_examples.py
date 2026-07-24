#!/usr/bin/env python3
"""Keep TYPE_ALGEBRA.md code examples in sync with examples/type_algebra/main.cpp.

examples/type_algebra/main.cpp is the single source of truth: CI builds and runs it,
proving that all examples are compilable. The regions bounded by `// sync-example-<name>`
are mirrored into the matching `<!-- sync-example-<name> -->` code fences in TYPE_ALGEBRA.md.
"""
import pathlib
import re
import sys

repo = pathlib.Path(__file__).resolve().parents[1]
example_path = repo / "examples" / "type_algebra" / "main.cpp"
doc_path = repo / "TYPE_ALGEBRA.md"

if not example_path.exists():
    sys.stderr.write(f"Error: {example_path} does not exist\n")
    sys.exit(1)

if not doc_path.exists():
    sys.stderr.write(f"Error: {doc_path} does not exist\n")
    sys.exit(1)

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
            sys.exit(1)
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
    sys.exit(1)

print("All TYPE_ALGEBRA.md code examples are fully synchronized!")
sys.exit(0)
