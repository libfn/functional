#!/bin/bash
# Install bazelisk (pinned + checksum-verified) and pre-fetch the pinned Bazel
# release so the test container can run Bazel. The system toolchain (CC/CXX) is
# used, so no in-image MODULE.bazel file.
set -ex
: "${BAZEL_RELEASE:?BAZEL_RELEASE is not set}"

BAZELISK_VERSION=1.29.0

ARCH=$(dpkg --print-architecture)
BASE="https://github.com/bazelbuild/bazelisk/releases/download/v${BAZELISK_VERSION}"
ASSET="bazelisk-linux-${ARCH}"

wget -O /tmp/bazelisk "${BASE}/${ASSET}"
wget -O /tmp/bazelisk.sha256 "${BASE}/${ASSET}.sha256"
# .sha256 holds a bare hash, so append the filename for `sha256sum -c`.
echo "$(cat /tmp/bazelisk.sha256)  /tmp/bazelisk" | sha256sum -c -

install -m 0755 /tmp/bazelisk /usr/local/bin/bazel
rm -f /tmp/bazelisk /tmp/bazelisk.sha256

# Pre-download the pinned Bazel into bazelisk's cache (no workspace needed).
USE_BAZEL_VERSION="${BAZEL_RELEASE}" bazel version
