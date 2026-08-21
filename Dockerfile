# Build stage: compiles the C++ project
FROM ubuntu:24.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
    && apt-get install --no-install-recommends -y \
    build-essential \
    ca-certificates \
    cmake \
    curl \
    git \
    libprotobuf-dev \
    libpython3-dev \
    protobuf-compiler \
    python3 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace

COPY . .

# Build ASSERTIONS=ON for an image intended for integration testing: same
# optimisation and same output as the default build, but internal invariant
# violations abort instead of yielding a verdict computed from corrupt state.
# Publish those under a separate tag - never as the release tag, since an
# assertion failure aborts the process.
ARG ASSERTIONS=OFF

RUN mkdir -p build \
    && cd build \
    && cmake .. -DCTORS_ASSERTIONS=${ASSERTIONS} \
    && cmake --build .


# Runtime stage: only the binary and its shared library dependencies
FROM ubuntu:24.04

ARG VERSION=0.0.0
LABEL org.opencontainers.image.source="https://github.com/Robust-Rail-NL/robust-rail-evaluator" \
      org.opencontainers.image.description="TORS evaluator" \
      org.opencontainers.image.version="${VERSION}" \
      org.opencontainers.image.licenses="Apache-2.0"

RUN apt-get update \
    && apt-get install --no-install-recommends -y libprotobuf32t64 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace

COPY --from=builder /workspace/build/TORS build/TORS
COPY --from=builder /workspace/build/cTORS/libcTORS.so /usr/local/lib/libcTORS.so
COPY examples_kleine_binckhorst/ examples_kleine_binckhorst/

RUN ldconfig \
    && chown -R ubuntu:ubuntu /workspace

USER ubuntu

ENTRYPOINT ["build/TORS"]
