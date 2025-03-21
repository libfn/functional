ARG GCC_RELEASE=14
ARG GCC_DIST=${GCC_RELEASE}-bookworm
FROM gcc:${GCC_DIST} AS gcc
RUN set -ex ;\
    find /usr/local/ -type f ;\
    cat /etc/ld.so.conf.d/* ;\
    cat /etc/os-release ;\
    /usr/local/bin/gcc --version

FROM debian:bookworm
COPY --from=gcc /usr/local/ /usr/local/
COPY --from=gcc /etc/ld.so.conf.d/ /etc/ld.so.conf.d/

WORKDIR /root
RUN set -ex ;\
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

ENV DEBIAN_FRONTEND=noninteractive
RUN set -ex ;\
    apt-get update ;\
    apt-get install -y --no-install-recommends \
      ca-certificates gpg gpg-agent wget vim curl jq \
      cmake ninja-build binutils libc6-dev git ;\
    apt-get clean

ENV CC=/usr/bin/gcc
ENV CXX=/usr/bin/g++
ENV CMAKE_GENERATOR=Ninja
ENV CMAKE_BUILD_TYPE=Debug

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
