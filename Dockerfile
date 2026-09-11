# Build stage: compiles the C++ project. Also published on its own as the
# ":devel" tag (see docker-push.sh) so .devcontainer/devcontainer.json can
# pull a ready-made toolchain image instead of rebuilding it from scratch.
FROM ubuntu:24.04 AS builder

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

WORKDIR /workspace

COPY . .

# Build ASSERTIONS=ON for an image intended for integration testing: same
# optimisation and same output as the default build, but internal invariant
# violations abort instead of yielding a verdict computed from corrupt state.
# Publish those under a separate tag - never as the release tag, since an
# assertion failure aborts the process.
ARG ASSERTIONS=OFF
# Empty by default, so a bare "docker build" with no --build-arg (i.e. not
# going through docker-push.sh/docker-push-edge.sh) still gets a sensible
# version: whatever's checked into the top-level CMakeLists.txt. When
# docker-push.sh/docker-push-edge.sh passes VERSION (the same value also put
# in the LABEL below), TORS_VERSION_OVERRIDE makes main.cpp's startup
# "TORS <version>" line - read from the binary's own compiled-in TORS_VERSION,
# not from the LABEL, which a running container can't introspect - actually
# match what the image claims to be. Previously it never did for edge builds:
# this ARG only reached the LABEL, so every image printed whatever version
# happened to be committed in CMakeLists.txt at build time, coincidentally
# correct for ordinary releases and silently wrong for edge builds (see
# docker-push-edge.sh).
ARG VERSION=

RUN mkdir -p build \
    && cd build \
    && cmake .. -DCTORS_ASSERTIONS=${ASSERTIONS} -DTORS_VERSION_OVERRIDE=${VERSION} \
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
COPY example_kleine_binckhorst/ example_kleine_binckhorst/

RUN ldconfig \
    && chown -R ubuntu:ubuntu /workspace

USER ubuntu

ENTRYPOINT ["build/TORS"]
