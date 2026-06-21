FROM debian:trixie-slim

ARG NODE_RELEASE=24
ARG DEBIAN_FRONTEND=noninteractive
WORKDIR /work
COPY fetch-node.sh /work/fetch-node.sh
RUN set -ex ;\
    echo "deb http://deb.debian.org/debian trixie-backports main" > /etc/apt/sources.list.d/backports.list ;\
    apt-get update ;\
    apt-get install -y --no-install-recommends \
      python3 python3-pip python3-venv \
      ca-certificates wget vim git curl gpg ;\
    apt-get install -y --no-install-recommends -t trixie-backports shellcheck ;\
    sh /work/fetch-node.sh ;\
    rm /work/fetch-node.sh ;\
    apt-get clean && rm -rf /var/lib/apt/lists/*

COPY pre-commit/requirements.txt /requirements.txt
ENV VENV=/venv
ENV PATH=${VENV}/bin:${PATH}
RUN set -ex ;\
    python3 -m venv ${VENV}  ;\
    pip --no-cache-dir install -r /requirements.txt ;\
    rm -rf /requirements.txt

WORKDIR /work
