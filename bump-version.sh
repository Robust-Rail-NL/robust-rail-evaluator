#!/usr/bin/env bash
# Bump the TORS version in CMakeLists.txt's project(VERSION ...) — the single
# source of truth used by docker-push.sh for the image tag and label.
#
# Also commits the change and creates a local, annotated git tag (vX.Y.Z),
# the same convention as `npm version`. Nothing is pushed — push the commit
# and tag yourself once you're happy with them, e.g.:
#   git push --follow-tags
set -euo pipefail

CMAKELISTS="CMakeLists.txt"

if [[ -n "$(git status --porcelain)" ]]; then
    echo "Working tree is not clean; commit or stash changes before bumping the version" >&2
    exit 1
fi

CURRENT=$(sed -n 's:.*project(TORS VERSION \([0-9.]*\)).*:\1:p' "$CMAKELISTS")
[[ -n "$CURRENT" ]] || { echo "Could not read project(TORS VERSION ...) from $CMAKELISTS" >&2; exit 1; }

usage() {
    echo "Usage: $0 <major|minor|patch|X.Y.Z>" >&2
    echo "Current version: $CURRENT" >&2
    exit 1
}

[[ $# -eq 1 ]] || usage

IFS='.' read -r MAJOR MINOR PATCH <<< "$CURRENT"

case "$1" in
    major) NEW="$((MAJOR + 1)).0.0" ;;
    minor) NEW="$MAJOR.$((MINOR + 1)).0" ;;
    patch) NEW="$MAJOR.$MINOR.$((PATCH + 1))" ;;
    [0-9]*.[0-9]*.[0-9]*) NEW="$1" ;;
    *) usage ;;
esac

sed -i "s/project(TORS VERSION $CURRENT)/project(TORS VERSION $NEW)/" "$CMAKELISTS"

git add "$CMAKELISTS"
git commit -m "Bump version to $NEW"
git tag -a "v$NEW" -m "v$NEW"

echo "Bumped version: $CURRENT -> $NEW"
echo "Created commit and tag v$NEW (not pushed — run 'git push --follow-tags' when ready)"
