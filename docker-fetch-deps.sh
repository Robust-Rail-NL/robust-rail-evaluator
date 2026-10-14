#!/usr/bin/env bash
# Clone the FetchContent dependencies the Docker build needs, once, outside
# CMake, so the (cached) Dockerfile "deps" stage does the network access and
# the real build never clones from github.com. Usage:
#   ./docker-fetch-deps.sh <dest-dir> <CMakeLists.txt>:<name> ...
# e.g. ./docker-fetch-deps.sh /deps cTORS/CMakeLists.txt:json
#
# The repository URL and tag are read from the dependency's own
# FetchContent_Declare(<name> ... GIT_REPOSITORY ... GIT_TAG ...) block, so the
# CMakeLists.txt files stay the single place a version is written down; a
# bump there is picked up here automatically. Fails if a block or field
# can't be found, rather than silently cloning something else.
set -euo pipefail

dest=$1
shift

field() { # <file> <name> <GIT_REPOSITORY|GIT_TAG>
    sed -n "/FetchContent_Declare($2\$/,/)/p" "$1" \
        | sed -n "s/.*$3[[:space:]]\{1,\}\([^[:space:])]\{1,\}\).*/\1/p" \
        | head -n1
}

for spec in "$@"; do
    file=${spec%%:*}
    name=${spec#*:}
    url=$(field "$file" "$name" GIT_REPOSITORY)
    tag=$(field "$file" "$name" GIT_TAG)
    [[ -n "$url" && -n "$tag" ]] || {
        echo "No GIT_REPOSITORY/GIT_TAG for FetchContent_Declare($name) in $file" >&2
        exit 1
    }
    echo "Cloning $name: $url @ $tag"
    git clone --quiet --depth 1 --branch "$tag" "$url" "$dest/$name"
done
