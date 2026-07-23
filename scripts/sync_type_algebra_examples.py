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

# We find <!-- sync-example-<name> --> followed by ```cpp <content> ```
# and replace the content with the region.
def replace_snippet(match):
    name = match.group(1)
    if name not in regions:
        sys.stderr.write(f"Warning: no matching region in main.cpp for sync-example-{name}\n")
        return match.group(0) # Keep unchanged

    return f"<!-- sync-example-{name} -->\n```cpp\n{regions[name]}\n```"

# Match: <!-- sync-example-(...) --> followed by whitespace and ```cpp ... ```
pattern = re.compile(r"<!-- sync-example-([\w-]+) -->\s*```cpp\n.*?```", re.DOTALL)
updated_text = pattern.sub(replace_snippet, doc_text)

# Clean up trailing whitespace in the document (like pre-commit trailing whitespace check does)
updated_text = re.sub(r"[ \t]+$", "", updated_text, flags=re.MULTILINE)

if doc_text != updated_text:
    doc_path.write_text(updated_text, encoding="utf-8")
    print(f"Synced code fences in {doc_path.relative_to(repo)} <- {example_path.relative_to(repo)}")
    sys.exit(1)

print("All TYPE_ALGEBRA.md code examples are fully synchronized!")
sys.exit(0)
