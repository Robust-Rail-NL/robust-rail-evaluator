FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
    && apt-get install --no-install-recommends -y \
    build-essential \
    ca-certificates \
    cmake \
    curl \
    gdb \
    git \
    libprotobuf-dev \
    libpython3-dev \
    protobuf-compiler \
    python3 \
    && rm -rf /var/lib/apt/lists/*

USER ubuntu

WORKDIR /workspace

CMD ["bash"]
