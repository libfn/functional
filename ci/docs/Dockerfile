ARG GCC_RELEASE=15
ARG DEBIAN_VERSION=trixie
ARG GCC_DIST=${GCC_RELEASE}-${DEBIAN_VERSION}
FROM gcc:${GCC_DIST} AS upstream
RUN set -ex ;\
    find /usr/local/ -type f ;\
    cat /etc/ld.so.conf.d/* ;\
    cat /etc/os-release ;\
    /usr/local/bin/gcc --version

ARG DEBIAN_VERSION=trixie
FROM debian:${DEBIAN_VERSION} AS build
ARG DOXYGEN_RELEASE=1.12.0
ARG ZNAI_RELEASE=1.73

ARG DEBIAN_FRONTEND=noninteractive
WORKDIR /work
RUN set -ex ;\
    apt-get update ;\
    apt-get install -y --no-install-recommends \
      ca-certificates wget cmake ninja-build git build-essential \
      python3 bison flex libiconv-hook-dev unzip ;\
    apt-get clean

ENV CMAKE_GENERATOR=Ninja
RUN set -ex ;\
    : "${DOXYGEN_RELEASE:?DOXYGEN_RELEASE is not set}" ;\
    DIST="https://www.doxygen.nl/files" ;\
    FILE="doxygen-${DOXYGEN_RELEASE}.src.tar.gz" ;\
    wget ${DIST}/${FILE} ;\
    tar -xzf ${FILE} ;\
    cd doxygen-${DOXYGEN_RELEASE} ;\
    cmake -B build -S . \
      -DCMAKE_BUILD_TYPE=Release ;\
    cmake --build build/ ;\
    cmake --build build/ --target install ;\
    cd /work ;\
    rm -rf ${FILE} doxygen-${DOXYGEN_RELEASE}

RUN set -ex ;\
    : "${ZNAI_RELEASE:?ZNAI_RELEASE is not set}" ;\
    DIST="https://repo.maven.apache.org/maven2/org/testingisdocumenting/znai/znai-dist" ;\
    FILE=znai-dist-${ZNAI_RELEASE}-znai.zip ;\
    wget ${DIST}/${ZNAI_RELEASE}/${FILE} ;\
    unzip ${FILE} -d znai_tmp ;\
    mv znai_tmp/dist /opt/znai ;\
    rm -rf ${FILE} znai_tmp

ARG DEBIAN_VERSION=trixie
FROM debian:${DEBIAN_VERSION}
COPY --from=upstream /usr/local/ /usr/local/
COPY --from=upstream /etc/ld.so.conf.d/ /etc/ld.so.conf.d/
COPY --from=build /usr/local/ /usr/local/
COPY --from=build /opt/znai /opt/znai

RUN set -ex ;\
    rm -rf /usr/local/lib/python3.11 ;\
    find /usr/local -type d -empty -exec rm -r {} + ;\
    ldconfig -v ;\
    dpkg-divert --divert /usr/bin/gcc.orig --rename /usr/bin/gcc ;\
    dpkg-divert --divert /usr/bin/g++.orig --rename /usr/bin/g++ ;\
    dpkg-divert --divert /usr/bin/gfortran.orig --rename /usr/bin/gfortran ;\
    update-alternatives --install /usr/bin/cc cc /usr/local/bin/gcc 999 ;\
    update-alternatives --install \
      /usr/bin/gcc gcc /usr/local/bin/gcc 100 \
      --slave /usr/bin/g++ g++ /usr/local/bin/g++ \
      --slave /usr/bin/gcc-ar gcc-ar /usr/local/bin/gcc-ar \
      --slave /usr/bin/gcc-nm gcc-nm /usr/local/bin/gcc-nm \
      --slave /usr/bin/gcc-ranlib gcc-ranlib /usr/local/bin/gcc-ranlib \
      --slave /usr/bin/gcov gcov /usr/local/bin/gcov \
      --slave /usr/bin/gcov-tool gcov-tool /usr/local/bin/gcov-tool \
      --slave /usr/bin/gcov-dump gcov-dump /usr/local/bin/gcov-dump \
      --slave /usr/bin/lto-dump lto-dump /usr/local/bin/lto-dump ;\
    update-alternatives --auto cc ;\
    update-alternatives --auto gcc

ARG NODE_RELEASE=24
ARG JAVA_VERSION=openjdk-21-jdk
ARG DEBIAN_FRONTEND=noninteractive
WORKDIR /work
COPY fetch-node.sh /work/fetch-node.sh
RUN set -ex ;\
    apt-get update ;\
    apt-get install -y --no-install-recommends \
      ca-certificates gnupg unzip wget vim curl jq \
      cmake ninja-build binutils libc6-dev git \
      ${JAVA_VERSION} graphviz  ;\
    bash /work/fetch-node.sh ;\
    rm /work/fetch-node.sh ;\
    apt-get clean && rm -rf /var/lib/apt/lists/*

ENV CC=/usr/bin/gcc
ENV CXX=/usr/bin/g++
ENV CMAKE_GENERATOR=Ninja
ENV CMAKE_BUILD_TYPE=Debug
ARG CATCH_RELEASE=3.14.0
COPY build-catch.sh /work/build-catch.sh
RUN set -ex ;\
    bash /work/build-catch.sh ;\
    rm /work/build-catch.sh
ENV CMAKE_FIND_CATCH2=1
ENV PATH=${PATH}:/opt/znai
