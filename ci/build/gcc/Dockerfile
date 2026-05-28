ARG GCC_RELEASE=16
ARG DEBIAN_VERSION=trixie
ARG GCC_DIST=${GCC_RELEASE}-${DEBIAN_VERSION}
FROM gcc:${GCC_DIST} AS upstream
RUN set -ex ;\
    find /usr/local/ -type f ;\
    cat /etc/ld.so.conf.d/* ;\
    cat /etc/os-release ;\
    /usr/local/bin/gcc --version

ARG DEBIAN_VERSION=trixie
FROM debian:${DEBIAN_VERSION} AS gcc
COPY --from=upstream /usr/local/ /usr/local/
COPY --from=upstream /etc/ld.so.conf.d/ /etc/ld.so.conf.d/

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
      ca-certificates gnupg unzip wget vim curl jq python3-venv python3-pip \
      cmake ninja-build binutils libc6-dev git valgrind ;\
    apt-get clean

COPY gcc/requirements.txt /requirements.txt
ENV VENV=/venv
ENV PATH=${VENV}/bin:${PATH}
RUN set -ex ;\
    python3 -m venv ${VENV}  ;\
    pip --no-cache-dir install -r /requirements.txt ;\
    rm -rf /requirements.txt

ENV CC=/usr/bin/gcc
ENV CXX=/usr/bin/g++
ENV CMAKE_GENERATOR=Ninja
ENV CMAKE_BUILD_TYPE=Debug

ARG CATCH_RELEASE=3.14.0
WORKDIR /work
COPY build-catch.sh /work/build-catch.sh
RUN set -ex ;\
    sh /work/build-catch.sh ;\
    rm /work/build-catch.sh
ENV CMAKE_FIND_CATCH2=1
