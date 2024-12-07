FROM debian:bookworm

ENV DEBIAN_FRONTEND=noninteractive
RUN set -ex ;\
    apt-get update ;\
    apt-get install -y --no-install-recommends \
      python3 python3-pip python3-venv \
      ca-certificates wget vim git ;\
    apt-get clean

COPY requirements.txt /requirements.txt
ENV VENV=/venv
ENV PATH=${VENV}/bin:${PATH}
RUN set -ex ;\
    python3 -m venv ${VENV}  ;\
    pip --no-cache-dir install -r requirements.txt

WORKDIR /work
