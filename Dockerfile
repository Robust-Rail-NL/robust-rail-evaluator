# syntax=docker/dockerfile:1

# Build stage: compiles the C++ project. Also published on its own as the
# ":devel" tag (see docker-push.sh) so .devcontainer/devcontainer.json can
# pull a ready-made toolchain image instead of rebuilding it from scratch.
FROM ubuntu:24.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
    && apt-get install --no-install-recommends -y \
    build-essential \
    ca-certificates \
    ccache \
    cmake \
    curl \
    gdb \
    git \
    libprotobuf-dev \
    libpython3-dev \
    protobuf-compiler \
    python3 \
    && rm -rf /var/lib/apt/lists/*

# TARGETARCH is one of BuildKit's automatic (but must-be-declared) build args.
# Used below to give amd64 and arm64 their own ccache cache-mount instead of
# sharing one: docker-push.sh builds both platforms concurrently on the same
# builder, and ccache entries for the two are never interchangeable anyway
# (different compiler target), so sharing an id would only add mount
# contention for no reuse benefit.
ARG TARGETARCH
ENV CCACHE_DIR=/root/.ccache
ENV CCACHE_MAXSIZE=2G

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

# --mount=type=cache persists /root/.ccache across builds on the same
# buildx builder (robust-rail-builder is shared/persistent across releases,
# not recreated per build — see docker-push.sh), so a rebuild that changes
# only VERSION or a handful of source files reuses ccache's object cache for
# everything else instead of recompiling from scratch. This is deliberately
# not the same mechanism as robust-rail-planner's registry build cache
# (--cache-to/--cache-from type=registry): that caches whole layers keyed on
# exact instruction inputs, which would never hit here since VERSION (and
# thus this RUN command's own arguments) changes every release; ccache
# caches per translation unit instead, so it stays useful even when this
# step's Docker-level cache is always a miss. It also won't help a
# completely fresh builder/machine the way the registry cache does — it's
# only warm as long as robust-rail-builder itself persists.
RUN --mount=type=cache,target=/root/.ccache,id=ccache-${TARGETARCH} \
    ./build.sh -DCTORS_ASSERTIONS=${ASSERTIONS} -DTORS_VERSION_OVERRIDE=${VERSION} \
    -DCMAKE_C_COMPILER_LAUNCHER=ccache -DCMAKE_CXX_COMPILER_LAUNCHER=ccache


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
