#!/usr/bin/env python3
"""Keep packaging version literals in sync with the VERSION file.

VERSION is the single source of truth. CMake and Conan read it dynamically, but
vcpkg and Bazel require a literal, so this hook mirrors it into:
  - ports/libfn/vcpkg.json  (version-semver; also resets port-version on change)
  - MODULE.bazel            (the module() version)

Both accept the full SemVer form (MAJOR.MINOR.PATCH[-prerelease][+build]), so the
VERSION value is copied verbatim. Run as a pre-commit hook: it rewrites the
literals and exits non-zero if it changed anything, so the commit is blocked
until the change is re-staged.
"""
import json
import pathlib
import re
import sys

SEMVER_RE = re.compile(
    r"^[0-9]+\.[0-9]+\.[0-9]+(-[0-9A-Za-z.-]+)?(\+[0-9A-Za-z.-]+)?$"
)
# The version string inside the module(...) directive (not a bazel_dep version).
MODULE_VERSION_RE = re.compile(r'(module\([^()]*?version\s*=\s*)"[^"]*"', re.DOTALL)

repo = pathlib.Path(__file__).resolve().parents[1]
version_path = repo / "VERSION"
vcpkg_path = repo / "ports" / "libfn" / "vcpkg.json"
module_path = repo / "MODULE.bazel"

version = version_path.read_text().strip()
if not SEMVER_RE.match(version):
    sys.stderr.write(
        f"{version_path.relative_to(repo)} must be SemVer 2.0.0 "
        f"(MAJOR.MINOR.PATCH[-prerelease][+build]); got {version!r}\n"
    )
    sys.exit(1)

changed = False

# vcpkg.json: mirror version-semver and reset the recipe-revision counter.
manifest = json.loads(vcpkg_path.read_text())
if manifest.get("version-semver") != version:
    manifest["version-semver"] = version
    manifest.pop("port-version", None)
    vcpkg_path.write_text(json.dumps(manifest, indent=2) + "\n")
    print(f"synced {vcpkg_path.relative_to(repo)} version-semver -> {version}")
    changed = True

# MODULE.bazel: mirror the module() version.
content = module_path.read_text()
new_content, n = MODULE_VERSION_RE.subn(lambda m: f'{m.group(1)}"{version}"', content, count=1)
if n == 0:
    sys.stderr.write(f"{module_path.relative_to(repo)}: could not find module() version to sync\n")
    sys.exit(1)
if new_content != content:
    module_path.write_text(new_content)
    print(f"synced {module_path.relative_to(repo)} module version -> {version}")
    changed = True

sys.exit(1 if changed else 0)
