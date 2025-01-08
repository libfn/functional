ARG GCC_RELEASE=14
ARG GCC_DIST=${GCC_RELEASE}-bookworm
FROM gcc:${GCC_DIST} AS gcc
RUN set -ex ;\
    find /usr/local/ -type f ;\
    cat /etc/ld.so.conf.d/* ;\
    cat /etc/os-release ;\
    /usr/local/bin/gcc --version

FROM debian:bookworm AS build
ARG DOXYGEN_RELEASE=1.12.0
ARG ZNAI_RELEASE=1.73

ENV CMAKE_GENERATOR=Ninja
ENV CMAKE_BUILD_TYPE=Debug
ENV DEBIAN_FRONTEND=noninteractive
WORKDIR /work
RUN set -ex ;\
    apt-get update ;\
    apt-get install -y --no-install-recommends \
      ca-certificates wget cmake ninja-build git build-essential \
      python3 bison flex libiconv-hook-dev unzip ;\
    apt-get clean

RUN set -ex ;\
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
    DIST="https://repo.maven.apache.org/maven2/org/testingisdocumenting/znai/znai-dist" ;\
    FILE=znai-dist-${ZNAI_RELEASE}-znai.zip ;\
    wget ${DIST}/${ZNAI_RELEASE}/${FILE} ;\
    unzip ${FILE} -d znai_tmp ;\
    mv znai_tmp/dist /opt/znai ;\
    rm -rf ${FILE} znai_tmp

FROM debian:bookworm
COPY --from=gcc /usr/local/ /usr/local/
COPY --from=gcc /etc/ld.so.conf.d/ /etc/ld.so.conf.d/
COPY --from=build /usr/local/ /usr/local/
COPY --from=build /opt/znai /opt/znai

WORKDIR /root
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

ENV PATH=${PATH}:/opt/znai
ENV CC=/usr/bin/gcc
ENV CXX=/usr/bin/g++
ENV CMAKE_GENERATOR=Ninja
ENV CMAKE_BUILD_TYPE=Debug
ENV DEBIAN_FRONTEND=noninteractive
RUN set -ex ;\
    apt-get update ;\
    apt-get install -y --no-install-recommends \
      ca-certificates gpg gpg-agent wget vim \
      cmake ninja-build binutils libc6-dev git \
      openjdk-17-jdk graphviz  ;\
    apt-get clean

ARG CATCH_RELEASE=3.7.1
WORKDIR /work
RUN set -ex ;\
    DIST="https://github.com/catchorg/Catch2/archive/refs/tags/" ;\
    FILE="v${CATCH_RELEASE}.tar.gz" ;\
    wget ${DIST}/${FILE} ;\
    tar -xzf ${FILE} ;\
    cd Catch2-${CATCH_RELEASE} ;\
    cmake -B build -S . \
      -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_TESTING=OFF \
      -DCATCH_DEVELOPMENT_BUILD=ON \
      -DCATCH_BUILD_TESTING=OFF ;\
    cmake --build build/ --target install ;\
    cd /work ;\
    rm -rf ${FILE} Catch2-${CATCH_RELEASE}

ENV CMAKE_FIND_CATCH2=1
