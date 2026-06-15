FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
    && apt-get install --no-install-recommends -y \
    build-essential \
    cmake \
    curl \
    git \
    libprotobuf-dev \
    protobuf-compiler \
    && rm -rf /var/lib/apt/lists/*

RUN groupadd --gid 1000 dev \
    && useradd --uid 1000 --gid 1000 -m dev

USER dev

WORKDIR /workspace

CMD ["bash"]
