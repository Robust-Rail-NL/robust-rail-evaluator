#!/usr/bin/env bash
# Build and push the multi-arch TORS evaluator image to ghcr.io.
#
# The version is read from CMakeLists.txt's project(TORS VERSION ...) plus
# TORS_VERSION_SUFFIX (the single source of truth — use bump-version.sh to
# change it) and passed into the image as a build-arg, so neither the
# Dockerfile LABEL nor the built binary's own startup "TORS <version>" line
# (TORS_VERSION_OVERRIDE, consumed by cTORS/CMakeLists.txt — see its comment)
# needs a separate edit.
#
# The :latest tag is only applied to final releases (no TORS_VERSION_SUFFIX).
# Prerelease versions (e.g. 2.0.0-alpha.4 on the noproto branch) are pushed
# under their own tag only, so they never shadow the current stable image.
#
# Two images are pushed per version: $VERSION, and $VERSION-assert built with
# -DCTORS_ASSERTIONS=ON. The pipeline in robust-rail-general evaluates
# every plan twice, once under each, and its --version stable-assert selector
# resolves to the -assert tag while keeping the generator and solver plain.
# Both tags are pushed together deliberately: when only the plain one existed,
# that selector referred to an image that had never been built, and the failure
# surfaced as a docker pull error long after the fact.
#
# A third, floating :devel image (the builder stage on its own, untagged by
# version) is also pushed — see the comment above that build for why.
#
# The -assert image is built for both architectures, like the plain one. It is
# tempting to call it a testing artifact and save the arm64 build, but we ship
# arm64, and the bugs an assertions build is best at catching — undefined
# behaviour, overflow, anything where the compiler was free to choose — are
# exactly the ones that can differ between architectures. Building assertions
# for amd64 only would leave the arm64 image both shipped and never
# assert-tested.
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
#
# --cache-to/--cache-from push and pull the build cache through a dedicated
# ":buildcache" tag, shared across all three builds below (plain, -assert,
# :devel all share the same builder-stage apt-get layer). See
# robust-rail-planner's docker-push.sh for the mechanism; ghcr.io/robust-rail-nl
# is public, so this costs no storage/bandwidth quota. It mainly benefits that
# apt-get layer, not the actual compile - see the Dockerfile's ccache
# cache-mount comment for why VERSION being baked into the compile command
# means this registry cache can't help there the way it does in
# robust-rail-planner.
#
# Refuses to run unless the working tree is clean and HEAD is the commit
# tagged v<version> (as created by bump-version.sh), with that tag pushed to
# origin. Pass -f to skip these checks.
set -euo pipefail
cd "$(dirname "$0")"

# BEGIN SHARED BLOCK: release-flags (identical in every repo's push scripts; edit all copies — checked by robust-rail-general/scripts/check_push_script_sync.py)
FORCE=0
while getopts f opt; do
    case $opt in
        f) FORCE=1 ;;
        *) echo "Usage: $0 [-f]" >&2; exit 2 ;;
    esac
done
# END SHARED BLOCK: release-flags

IMAGE="ghcr.io/robust-rail-nl/tors"
CACHE_REF="$IMAGE:buildcache"
BUILDER_NAME="robust-rail-builder"

RELEASE=$(sed -n 's:.*project(TORS VERSION \([0-9.]*\)).*:\1:p' CMakeLists.txt)
[[ -n "$RELEASE" ]] || { echo "Could not read project(TORS VERSION ...) from CMakeLists.txt" >&2; exit 1; }
SUFFIX=$(sed -n 's:.*set(TORS_VERSION_SUFFIX "\(.*\)").*:\1:p' CMakeLists.txt)

VERSION="$RELEASE"
[[ -n "$SUFFIX" ]] && VERSION="$RELEASE-$SUFFIX"

# BEGIN SHARED BLOCK: release-guards (identical in every repo's push scripts; edit all copies — checked by robust-rail-general/scripts/check_push_script_sync.py)
# A release image is permanent, and :$VERSION / :latest is what consumers
# pull, so only publish a commit that bump-version.sh has tagged v$VERSION
# and whose tag is on origin (which also means the commit itself is pushed).
# The working tree must be clean too: the docker build uses the working
# tree, not the commit. -f skips all of these checks.
TAG="v$VERSION"
if [[ $FORCE -eq 1 ]]; then
    echo "WARNING: -f given, skipping the clean-tree / pushed-tag checks for $TAG." >&2
else
    if [[ -n "$(git status --porcelain)" ]]; then
        echo "Refusing to publish $VERSION from a dirty working tree — commit or stash first (or use -f)." >&2
        git status --short >&2
        exit 1
    fi
    TAG_SHA=$(git rev-parse -q --verify "refs/tags/$TAG^{commit}") || {
        echo "Refusing to publish $VERSION: no local tag $TAG — run bump-version.sh, or use -f." >&2
        exit 1
    }
    if [[ "$TAG_SHA" != "$(git rev-parse HEAD)" ]]; then
        echo "Refusing to publish $VERSION: HEAD ($(git rev-parse --short HEAD)) is not the commit tagged $TAG ($(git rev-parse --short "$TAG_SHA")) — checkout $TAG, or use -f." >&2
        exit 1
    fi
    # Annotated tags list the tagged commit on the peeled ^{} line; a
    # lightweight tag has only the plain line.
    REMOTE_SHA=$(git ls-remote origin "refs/tags/$TAG^{}" | cut -f1 || true)
    [[ -n "$REMOTE_SHA" ]] || REMOTE_SHA=$(git ls-remote origin "refs/tags/$TAG" | cut -f1 || true)
    if [[ "$REMOTE_SHA" != "$TAG_SHA" ]]; then
        echo "Refusing to publish $VERSION: tag $TAG is not on origin at $(git rev-parse --short "$TAG_SHA") (or origin is unreachable) — git push origin $TAG, or use -f." >&2
        exit 1
    fi
fi
# END SHARED BLOCK: release-guards

# BEGIN SHARED BLOCK: ghcr-login (identical in every repo's push scripts; edit all copies — checked by robust-rail-general/scripts/check_push_script_sync.py)
docker login ghcr.io
# Only the push needs auth; ghcr.io/robust-rail-nl is public for reads. Log
# out again once this script exits (success or failure) rather than leaving
# the credential sitting in the credential store, where every unrelated
# docker pull against ghcr.io (e.g. run_experiment.py's) ends up decrypting
# it for no reason.
trap 'docker logout ghcr.io' EXIT
# END SHARED BLOCK: ghcr-login

TAGS=(-t "$IMAGE:$VERSION")
[[ -z "$SUFFIX" ]] && TAGS+=(-t "$IMAGE:latest")

# BEGIN SHARED BLOCK: buildx-builder (identical in every repo's push scripts; edit all copies — checked by robust-rail-general/scripts/check_push_script_sync.py)
if ! docker buildx inspect "$BUILDER_NAME" >/dev/null 2>&1; then
    docker buildx create --name "$BUILDER_NAME" --driver docker-container --driver-opt network=host
fi
# END SHARED BLOCK: buildx-builder

docker buildx build \
    --builder "$BUILDER_NAME" \
    --platform linux/amd64,linux/arm64 \
    --build-arg "VERSION=$VERSION" \
    "${TAGS[@]}" \
    --cache-to "type=registry,ref=$CACHE_REF,mode=max" \
    --cache-from "type=registry,ref=$CACHE_REF" \
    --push \
    .

TAGS=(-t "$IMAGE:$VERSION-assert")
[[ -z "$SUFFIX" ]] && TAGS+=(-t "$IMAGE:assert")

# Never tagged :latest, whatever the version shape — :latest is what someone
# gets when they ask for the evaluator without thinking about it, and that
# should never be a build that aborts on an internal invariant.
docker buildx build \
    --builder "$BUILDER_NAME" \
    --platform linux/amd64,linux/arm64 \
    --build-arg "VERSION=$VERSION" \
    --build-arg "ASSERTIONS=ON" \
    "${TAGS[@]}" \
    --cache-to "type=registry,ref=$CACHE_REF,mode=max" \
    --cache-from "type=registry,ref=$CACHE_REF" \
    --push \
    .

# The builder stage on its own, published as a floating :devel tag that
# .devcontainer/devcontainer.json pulls instead of building the toolchain
# image from scratch. Floating (not $VERSION-devel) and always overwritten:
# it's a dev-tooling image, not a reproducible release artifact, so tracking
# the release version would only leave devcontainer.json stale after every
# bump. Multi-arch for the same reason as the images above — several
# developers work on arm64.
docker buildx build \
    --builder "$BUILDER_NAME" \
    --platform linux/amd64,linux/arm64 \
    --target builder \
    -t "$IMAGE:devel" \
    --cache-to "type=registry,ref=$CACHE_REF,mode=max" \
    --cache-from "type=registry,ref=$CACHE_REF" \
    --push \
    .
