FROM debian:trixie-slim

ARG DEBIAN_FRONTEND=noninteractive
RUN set -ex ;\
    echo "deb http://deb.debian.org/debian trixie-backports main" > /etc/apt/sources.list.d/backports.list ;\
    apt-get update ;\
    apt-get install -y --no-install-recommends \
      python3 python3-pip python3-venv \
      ca-certificates wget vim git ;\
    apt-get install -y --no-install-recommends -t trixie-backports shellcheck ;\
    apt-get clean

COPY requirements.txt /requirements.txt
ENV VENV=/venv
ENV PATH=${VENV}/bin:${PATH}
RUN set -ex ;\
    python3 -m venv ${VENV}  ;\
    pip --no-cache-dir install -r /requirements.txt ;\
    rm -rf /requirements.txt

WORKDIR /work
