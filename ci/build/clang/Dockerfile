FROM debian:bookworm

ARG CLANG_RELEASE=19
ENV CLANG_RELEASE=${CLANG_RELEASE}
ENV DEBIAN_FRONTEND=noninteractive
RUN set -ex ;\
    apt-get update ;\
    apt-get install -y --no-install-recommends \
      ca-certificates gpg gpg-agent wget vim \
      cmake ninja-build binutils libc6-dev git ;\
    CODENAME=$( . /etc/os-release && echo $VERSION_CODENAME ) ;\
    wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | gpg --dearmor -o /etc/apt/keyrings/llvm.gpg ;\
    printf "%s\n%s\n" \
      "deb [signed-by=/etc/apt/keyrings/llvm.gpg] https://apt.llvm.org/${CODENAME}/ llvm-toolchain-${CODENAME}-${CLANG_RELEASE} main" \
      "deb-src [signed-by=/etc/apt/keyrings/llvm.gpg] https://apt.llvm.org/${CODENAME}/ llvm-toolchain-${CODENAME}-${CLANG_RELEASE} main" \
      | tee /etc/apt/sources.list.d/llvm.list ;\
    apt-get update ;\
    apt-get install -t llvm-toolchain-${CODENAME}-${CLANG_RELEASE} -y --no-install-recommends \
      clang++-${CLANG_RELEASE} clang-${CLANG_RELEASE} libc++-${CLANG_RELEASE}-dev libc++abi-${CLANG_RELEASE}-dev ;\
    apt-get clean

RUN set -ex ;\
    update-alternatives --install \
      /usr/bin/clang clang /usr/bin/clang-${CLANG_RELEASE} 100 \
      --slave /usr/bin/clang++ clang++ /usr/bin/clang++-${CLANG_RELEASE} ;\
    update-alternatives --auto clang

ENV CC=/usr/bin/clang
ENV CXX=/usr/bin/clang++
ENV CXXFLAGS="-stdlib=libc++"
ENV CMAKE_GENERATOR=Ninja
ENV CMAKE_BUILD_TYPE=Debug

WORKDIR /work
ARG CATCH_RELEASE=3.7.1
ENV CATCH_RELEASE=${CATCH_RELEASE}
RUN set -ex ;\
    git clone https://github.com/catchorg/Catch2.git ;\
    cd Catch2 ;\
    git checkout v${CATCH_RELEASE} ;\
    cmake -B build -S . \
      -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_TESTING=OFF \
      -DCATCH_DEVELOPMENT_BUILD=ON \
      -DCATCH_BUILD_TESTING=OFF ;\
    cmake --build build/ --target install ;\
    cd /work ;\
    rm -rf Catch2

ENV CMAKE_FIND_CATCH2=1
