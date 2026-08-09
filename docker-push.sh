#!/usr/bin/env bash
# Build and push the multi-arch TORS evaluator image to ghcr.io.
#
# The version is read from CMakeLists.txt's project(TORS VERSION ...) plus
# TORS_VERSION_SUFFIX (the single source of truth — use bump-version.sh to
# change it) and passed into the image as a build-arg, so the Dockerfile
# LABEL never needs a separate edit.
#
# The :latest tag is only applied to final releases (no TORS_VERSION_SUFFIX).
# Prerelease versions (e.g. 2.0.0-alpha.4 on the noproto branch) are pushed
# under their own tag only, so they never shadow the current stable image.
#
# Two images are pushed per version: $VERSION, and $VERSION-assert built with
# -DCTORS_ASSERTIONS=ON. The pipeline in scenario-planning-inputs evaluates
# every plan twice, once under each, and its --version 2.0.0-assert selector
# resolves to the -assert tag while keeping the generator and solver plain.
# Both tags are pushed together deliberately: when only the plain one existed,
# that selector referred to an image that had never been built, and the failure
# surfaced as a docker pull error long after the fact. The -assert image is
# amd64-only — it is a testing artifact, never deployed, and building it for
# two architectures doubles the release build for no one's benefit.
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

RELEASE=$(sed -n 's:.*project(TORS VERSION \([0-9.]*\)).*:\1:p' CMakeLists.txt)
[[ -n "$RELEASE" ]] || { echo "Could not read project(TORS VERSION ...) from CMakeLists.txt" >&2; exit 1; }
SUFFIX=$(sed -n 's:.*set(TORS_VERSION_SUFFIX "\(.*\)").*:\1:p' CMakeLists.txt)

VERSION="$RELEASE"
[[ -n "$SUFFIX" ]] && VERSION="$RELEASE-$SUFFIX"

TAGS=(-t "$IMAGE:$VERSION")
[[ -z "$SUFFIX" ]] && TAGS+=(-t "$IMAGE:latest")

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

# Never tagged :latest, whatever the version shape — :latest is what someone
# gets when they ask for the evaluator without thinking about it, and that
# should never be a build that aborts on an internal invariant.
docker buildx build \
    --builder "$BUILDER_NAME" \
    --platform linux/amd64 \
    --build-arg "VERSION=$VERSION" \
    --build-arg "ASSERTIONS=ON" \
    -t "$IMAGE:$VERSION-assert" \
    --push \
    .
