#!/bin/sh
# Install bazelisk and pre-fetch the pinned Bazel release so the test container can
# run Bazel. The system toolchain (CC/CXX) is used, so no in-image MODULE.bazel file.
set -ex
: "${BAZEL_RELEASE:?BAZEL_RELEASE is not set}"

ARCH=$(dpkg --print-architecture)
wget -O /usr/local/bin/bazel \
  "https://github.com/bazelbuild/bazelisk/releases/latest/download/bazelisk-linux-${ARCH}"
chmod +x /usr/local/bin/bazel

# Pre-download the pinned Bazel into bazelisk's cache (no workspace needed).
USE_BAZEL_VERSION="${BAZEL_RELEASE}" bazel version
