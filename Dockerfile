# Build stage: installs toolchain, conda, and compiles the C++ project
FROM ubuntu:20.04 AS builder

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

COPY . .

RUN conda env create -f env.yml && conda init

RUN apt-get update && apt-get install -y \
    git \
    && rm -rf /var/lib/apt/lists/*

RUN bash /workspace/build.sh


# Runtime stage: only the binary and its shared library dependencies
FROM ubuntu:20.04

LABEL org.opencontainers.image.source="https://github.com/Robust-Rail-NL/robust-rail-evaluator" \
      org.opencontainers.image.description="TORS evaluator" \
      org.opencontainers.image.version="0.2" \
      org.opencontainers.image.licenses="Apache-2.0"

WORKDIR /workspace

COPY --from=builder /workspace/build/TORS build/TORS
COPY --from=builder /workspace/build/cTORS/libcTORS.so /usr/local/lib/libcTORS.so
COPY --from=builder /opt/conda/envs/my_proto_env/lib/libprotobuf.so* /usr/local/lib/
COPY data/ data/

RUN ldconfig

ENTRYPOINT ["build/TORS"]
