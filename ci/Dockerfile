FROM fedora:39

WORKDIR /root

RUN dnf -y update ;\
    dnf -y install redhat-lsb python3.11 gcc libstdc++-static ccache \
    vim wget sudo curl git grep sed gdb zsh cmake doxygen ninja-build \
    unzip java-latest-openjdk clang git-clang-format clang-tools-extra llvm ;\
    dnf -y clean all

# Fail build if unexpected gcc version
RUN gcc --version | grep -E "^gcc \(GCC\) 13"

RUN wget https://repo.maven.apache.org/maven2/org/testingisdocumenting/znai/znai-dist/1.70/znai-dist-1.70-znai.zip ;\
    unzip znai-dist-1.70-znai.zip -d znai_tmp; mv znai_tmp/dist /opt/znai ;\
    rm -rf znai-dist-1.70-znai.zip znai_tmp
ENV PATH=/opt/znai:${PATH}

ENV CC=/usr/bin/gcc
ENV CXX=/usr/bin/g++
ENV CMAKE_GENERATOR='Ninja'
ENV CMAKE_C_COMPILER=${CC}
ENV CMAKE_CXX_COMPILER=${CXX}
