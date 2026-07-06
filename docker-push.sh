#!/usr/bin/env bash
# Build and push the multi-arch TORS evaluator image to ghcr.io.
#
# The version is read from CMakeLists.txt's project(TORS VERSION ...) (the
# single source of truth — use bump-version.sh to change it) and passed into
# the image as a build-arg, so the Dockerfile LABEL never needs a separate
# edit.
#
# :latest is applied unconditionally for now — this repo doesn't yet have a
# prerelease/stable branch split like robust-rail-solver's dev/noproto.
# Revisit this (gate :latest behind a version-shape regex, as solver does)
# once a prerelease line exists here, e.g. when migrating away from
# protobuf.
#
# Requires a buildx builder using the "docker-container" driver with
# network=host. The default driver runs the BuildKit container in an
# isolated network namespace whose DNS resolution can fail to reach
# private/LAN DNS servers (seen as: "docker build" works, "docker buildx
# build" times out resolving a private host). network=host makes the
# builder share the host's network stack, avoiding that failure mode.
#
# BUILDER_NAME is shared with sibling Robust-Rail-NL projects (e.g.
# robust-rail-solver) that need the same multi-arch/network=host setup — a
# buildx builder isn't tied to a specific repo or Dockerfile.
set -euo pipefail

IMAGE="ghcr.io/robust-rail-nl/tors"
BUILDER_NAME="robust-rail-builder"

VERSION=$(sed -n 's:.*project(TORS VERSION \([0-9.]*\)).*:\1:p' CMakeLists.txt)
[[ -n "$VERSION" ]] || { echo "Could not read project(TORS VERSION ...) from CMakeLists.txt" >&2; exit 1; }

TAGS=(-t "$IMAGE:$VERSION" -t "$IMAGE:latest")

if ! docker buildx inspect "$BUILDER_NAME" >/dev/null 2>&1; then
    docker buildx create --name "$BUILDER_NAME" --driver docker-container --driver-opt network=host
fi

docker buildx build \
    --builder "$BUILDER_NAME" \
    --platform linux/amd64,linux/arm64 \
    --build-arg "VERSION=$VERSION" \
    "${TAGS[@]}" \
    --push \
    .
