FROM ubuntu:20.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
    && apt-get install --no-install-recommends -y \
    build-essential \
    curl \
    git \
    software-properties-common \
    && add-apt-repository -y ppa:ubuntu-toolchain-r/test \
    && apt-get update \
    && apt-get install --no-install-recommends -y \
    gcc-9 g++-9 cmake \
    && rm -rf /var/lib/apt/lists/*

RUN update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-9 100 \
    && update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-9 100

RUN groupadd --gid 1000 dev \
    && useradd --uid 1000 --gid 1000 -m dev \
    && chgrp dev /opt \
    && chmod g+w /opt

ARG TARGETARCH

USER dev

RUN if [ "$TARGETARCH" = "amd64" ]; then \
        MINIFORGE_ARCH="x86_64"; \
    elif [ "$TARGETARCH" = "arm64" ]; then \
        MINIFORGE_ARCH="aarch64"; \
    else \
        echo "Unsupported architecture: $TARGETARCH" && exit 1; \
    fi \
    && curl -L -o /tmp/Miniforge.sh "https://github.com/conda-forge/miniforge/releases/latest/download/Miniforge3-Linux-${MINIFORGE_ARCH}.sh" \
    && bash /tmp/Miniforge.sh -b -p /opt/conda \
    && rm /tmp/Miniforge.sh

ENV PATH="/opt/conda/bin:$PATH"

WORKDIR /workspace

CMD ["bash"]
