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

ARG TARGETARCH
RUN if [ "$TARGETARCH" = "amd64" ]; then \
        MINIFORGE_ARCH="x86_64"; \
    elif [ "$TARGETARCH" = "arm64" ]; then \
        MINIFORGE_ARCH="aarch64"; \
    else \
        echo "Unsupported architecture: $TARGETARCH" && exit 1; \
    fi \
    && curl -L -o Miniforge.sh "https://github.com/conda-forge/miniforge/releases/latest/download/Miniforge3-Linux-${MINIFORGE_ARCH}.sh" \
    && bash Miniforge.sh -b -p /opt/conda \
    && rm Miniforge.sh

ENV PATH="/opt/conda/bin:$PATH"

WORKDIR /workspace

CMD ["bash"]
