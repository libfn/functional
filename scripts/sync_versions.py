#!/usr/bin/env python3
"""Keep packaging version literals in sync with the VERSION file.

VERSION is the single source of truth. CMake and Conan read it dynamically, but
vcpkg and Bazel require a literal, so this hook mirrors it into:
  - ports/libfn/vcpkg.json    (version-semver; also resets port-version on change)
  - MODULE.bazel              (the module() version)
  - include/libfn_version.hpp (the LIBFN_VERSION inline-namespace spellings)

vcpkg and Bazel accept the full SemVer form (MAJOR.MINOR.PATCH[-prerelease][+build]),
so the VERSION value is copied verbatim. The namespace spellings are derived:
0.y lines with y >= 1 share v0_<y> (z bumps are ABI-compatible), the 0.0.z line
versions per patch, a prerelease is appended (-dev -> _dev), and the _cxx26 twin
keeps _cxx26 last. LIBFN_VERSION_BASE is the mode-less spelling (pfn wraps in it,
regardless of LIBFN_CXX26). Run as a pre-commit hook: it rewrites the literals and
exits non-zero if it changed anything, so the commit is blocked until the change is
re-staged.
"""
import json
import pathlib
import re
import sys

SEMVER_RE = re.compile(
    r"^(0|[1-9]\d*)\.(0|[1-9]\d*)\.(0|[1-9]\d*)(?:-((?:0|[1-9]\d*|\d*[a-zA-Z-][0-9a-zA-Z-]*)(?:\.(?:0|[1-9]\d*|\d*[a-zA-Z-][0-9a-zA-Z-]*))*))?(?:\+([0-9a-zA-Z-]+(?:\.[0-9a-zA-Z-]+)*))?$"
)
# The version string inside the module(...) directive (not a bazel_dep version).
MODULE_VERSION_RE = re.compile(r'(module\([^()]*?version\s*=\s*)"[^"]*"', re.DOTALL)
# The #define lines of libfn_version.hpp, anchored to their preprocessor context.
HEADER_BASE_RE = re.compile(r"(#define LIBFN_VERSION_BASE )\w+")
HEADER_CXX26_RE = re.compile(r"(#ifdef LIBFN_CXX26\n#define LIBFN_VERSION )\w+")
HEADER_PLAIN_RE = re.compile(r"(#else\n#define LIBFN_VERSION )\w+")

repo = pathlib.Path(__file__).resolve().parents[1]
version_path = repo / "VERSION"
vcpkg_path = repo / "ports" / "libfn" / "vcpkg.json"
module_path = repo / "MODULE.bazel"
header_path = repo / "include" / "libfn_version.hpp"

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

# libfn_version.hpp: derive the inline-namespace spellings (see module docstring).
major, minor, patch, prerelease = SEMVER_RE.match(version).groups()[:4]
namespace = f"v{major}_{minor}" if not (major == "0" and minor == "0") else f"v0_0_{patch}"
if prerelease:
    namespace += "_" + re.sub(r"[^0-9a-zA-Z]", "_", prerelease)

content = header_path.read_text()
new_content, n_base = HEADER_BASE_RE.subn(rf"\g<1>{namespace}", content, count=1)
new_content, n_cxx26 = HEADER_CXX26_RE.subn(rf"\g<1>{namespace}_cxx26", new_content, count=1)
new_content, n_plain = HEADER_PLAIN_RE.subn(rf"\g<1>{namespace}", new_content, count=1)
if n_base == 0 or n_cxx26 == 0 or n_plain == 0:
    sys.stderr.write(f"{header_path.relative_to(repo)}: could not find the LIBFN_VERSION defines to sync\n")
    sys.exit(1)
if new_content != content:
    header_path.write_text(new_content)
    print(f"synced {header_path.relative_to(repo)} LIBFN_VERSION -> {namespace}")
    changed = True

sys.exit(1 if changed else 0)
