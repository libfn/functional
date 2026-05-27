#!/usr/bin/env python3
"""Keep ports/libfn/vcpkg.json's version-semver in sync with the VERSION file.

VERSION is the single source of truth. CMake and Conan read it dynamically;
vcpkg cannot, so this hook mirrors it into the port manifest. Also resets
port-version (a per-upstream-version recipe-revision counter) whenever the
upstream version changes.
"""
import json
import pathlib
import re
import sys

SEMVER_RE = re.compile(
    r"^[0-9]+\.[0-9]+\.[0-9]+(-[0-9A-Za-z.-]+)?(\+[0-9A-Za-z.-]+)?$"
)

repo = pathlib.Path(__file__).resolve().parents[1]
version_path = repo / "VERSION"
manifest_path = repo / "ports" / "libfn" / "vcpkg.json"

version = version_path.read_text().strip()
if not SEMVER_RE.match(version):
    sys.stderr.write(
        f"{version_path.relative_to(repo)} must be SemVer 2.0.0 "
        f"(MAJOR.MINOR.PATCH[-prerelease][+build]); got {version!r}\n"
    )
    sys.exit(1)

manifest = json.loads(manifest_path.read_text())
if manifest.get("version-semver") == version:
    sys.exit(0)

manifest["version-semver"] = version
manifest.pop("port-version", None)
manifest_path.write_text(json.dumps(manifest, indent=2) + "\n")
print(f"synced {manifest_path.relative_to(repo)} version-semver -> {version}")
sys.exit(1)
