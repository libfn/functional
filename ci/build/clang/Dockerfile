ARG DEBIAN_VERSION=trixie
FROM debian:${DEBIAN_VERSION} AS base

ARG CLANG_RELEASE=22
ENV CLANG_RELEASE=${CLANG_RELEASE}
ENV DEBIAN_FRONTEND=noninteractive
RUN set -ex ;\
    apt-get update ;\
    apt-get install -y --no-install-recommends \
      ca-certificates gnupg unzip wget vim curl jq python3-venv python3-pip \
      cmake ninja-build binutils libc6-dev git valgrind ;\
    CODENAME=$( . /etc/os-release && echo $VERSION_CODENAME ) ;\
    wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | gpg --dearmor -o /etc/apt/keyrings/llvm.gpg ;\
    printf "%s\n%s\n" \
      "deb [signed-by=/etc/apt/keyrings/llvm.gpg] https://apt.llvm.org/${CODENAME}/ llvm-toolchain-${CODENAME}-${CLANG_RELEASE} main" \
      "deb-src [signed-by=/etc/apt/keyrings/llvm.gpg] https://apt.llvm.org/${CODENAME}/ llvm-toolchain-${CODENAME}-${CLANG_RELEASE} main" \
      | tee /etc/apt/sources.list.d/llvm.list ;\
    apt-get update ;\
    apt-get install -t llvm-toolchain-${CODENAME}-${CLANG_RELEASE} -y --no-install-recommends \
      clang++-${CLANG_RELEASE} clang-${CLANG_RELEASE} libc++-${CLANG_RELEASE}-dev \
      libc++abi-${CLANG_RELEASE}-dev libclang-rt-${CLANG_RELEASE}-dev ;\
    apt-get clean && rm -rf /var/lib/apt/lists/*

RUN set -ex ;\
    update-alternatives --install \
      /usr/bin/clang clang /usr/bin/clang-${CLANG_RELEASE} 100 \
      --slave /usr/bin/clang++ clang++ /usr/bin/clang++-${CLANG_RELEASE} ;\
    update-alternatives --auto clang

COPY clang/requirements.txt /requirements.txt
ENV VENV=/venv
ENV PATH=${VENV}/bin:${PATH}
RUN set -ex ;\
    python3 -m venv ${VENV}  ;\
    pip --no-cache-dir install -r /requirements.txt ;\
    rm -rf /requirements.txt

ENV CC=/usr/bin/clang
ENV CXX=/usr/bin/clang++
ENV CMAKE_GENERATOR=Ninja
ENV CMAKE_BUILD_TYPE=Debug

# ---- Build Catch2 with libc++ as `clang` target ----

FROM base AS clang
ARG CATCH_RELEASE=3.14.0

WORKDIR /work
COPY build-catch.sh /work/build-catch.sh
ENV CXXFLAGS="-stdlib=libc++"
RUN set -ex ;\
    sh /work/build-catch.sh ;\
    rm /work/build-catch.sh
ENV CMAKE_FIND_CATCH2=1

# ---- Build Catch2 with libstdc++ based on gcc-14, as `clang-libstdcxx` target ----

FROM base AS clang-libstdcxx
ARG CATCH_RELEASE=3.14.0

RUN set -ex ;\
    apt-get update ;\
    apt-get install -y --no-install-recommends libstdc++-14-dev ;\
    apt-get clean && rm -rf /var/lib/apt/lists/*

WORKDIR /work
COPY build-catch.sh /work/build-catch.sh
ENV CXXFLAGS="-stdlib=libstdc++"
RUN set -ex ;\
    sh /work/build-catch.sh ;\
    rm /work/build-catch.sh
ENV CMAKE_FIND_CATCH2=1

# ---- Bazel + system clang as `clang-bazel` target ----

FROM base AS clang-bazel
ARG BAZEL_RELEASE=7.7.1

# Install libstdc++ so clang libstdc++ (i.e. default) Bazel builds work; base has only libc++
RUN set -ex ;\
    apt-get update ;\
    apt-get install -y --no-install-recommends libstdc++-14-dev ;\
    apt-get clean && rm -rf /var/lib/apt/lists/*

WORKDIR /work
COPY fetch-bazel.sh /work/fetch-bazel.sh
RUN set -ex ;\
    sh /work/fetch-bazel.sh ;\
    rm /work/fetch-bazel.sh
