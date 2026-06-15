# Build stage: compiles the C++ project
FROM ubuntu:24.04 AS builder

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

WORKDIR /workspace

COPY . .

RUN bash /workspace/build.sh


# Runtime stage: only the binary and its shared library dependencies
FROM ubuntu:24.04

LABEL org.opencontainers.image.source="https://github.com/Robust-Rail-NL/robust-rail-evaluator" \
      org.opencontainers.image.description="TORS evaluator" \
      org.opencontainers.image.version="0.2" \
      org.opencontainers.image.licenses="Apache-2.0"

RUN apt-get update \
    && apt-get install --no-install-recommends -y libprotobuf32t64 \
    && rm -rf /var/lib/apt/lists/*

RUN groupadd --gid 999 tors \
    && useradd --uid 999 --gid 999 -m tors

WORKDIR /workspace

COPY --from=builder /workspace/build/TORS build/TORS
COPY --from=builder /workspace/build/cTORS/libcTORS.so /usr/local/lib/libcTORS.so
COPY data/ data/

RUN ldconfig \
    && chown -R 999:999 /workspace

USER tors

ENTRYPOINT ["build/TORS"]
