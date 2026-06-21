#!/bin/bash
set -ex
set -o pipefail
: "${CLANG_RELEASE:?CLANG_RELEASE is not set}"
: "${CODENAME:?CODENAME is not set}"

mkdir -p /etc/apt/keyrings
curl --proto '=https' --tlsv1.2 -fsSL https://apt.llvm.org/llvm-snapshot.gpg.key \
  | gpg --dearmor -o /etc/apt/keyrings/llvm.gpg
printf "%s\n%s\n" \
  "deb [signed-by=/etc/apt/keyrings/llvm.gpg] https://apt.llvm.org/${CODENAME}/ llvm-toolchain-${CODENAME}-${CLANG_RELEASE} main" \
  "deb-src [signed-by=/etc/apt/keyrings/llvm.gpg] https://apt.llvm.org/${CODENAME}/ llvm-toolchain-${CODENAME}-${CLANG_RELEASE} main" \
  | tee /etc/apt/sources.list.d/llvm.list
apt-get update
apt-get install -t llvm-toolchain-${CODENAME}-${CLANG_RELEASE} -y --no-install-recommends \
  clang++-${CLANG_RELEASE} clang-${CLANG_RELEASE} libc++-${CLANG_RELEASE}-dev \
  libc++abi-${CLANG_RELEASE}-dev libclang-rt-${CLANG_RELEASE}-dev
