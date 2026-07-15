#!/usr/bin/env bash
# Bump the TORS version tracked in CMakeLists.txt.
#
# The release number lives in project(TORS VERSION X.Y.Z) (CMake's
# project(VERSION ...) is numeric-only — it rejects prerelease suffixes like
# "-alpha.1"). Any prerelease suffix is tracked separately in
# TORS_VERSION_SUFFIX, right below it. docker-push.sh combines the two into
# the full version string ("X.Y.Z" or "X.Y.Z-suffix") used for the image tag
# and label.
#
# Also commits the change and creates a local, annotated git tag
# (vX.Y.Z[-suffix]), the same convention as `npm version`. Nothing is
# pushed — push the commit and tag yourself once you're happy with them:
#   git push --follow-tags
set -euo pipefail

CMAKELISTS="CMakeLists.txt"

if [[ -n "$(git status --porcelain)" ]]; then
    echo "Working tree is not clean; commit or stash changes before bumping the version" >&2
    exit 1
fi

RELEASE=$(sed -n 's:.*project(TORS VERSION \([0-9.]*\)).*:\1:p' "$CMAKELISTS")
[[ -n "$RELEASE" ]] || { echo "Could not read project(TORS VERSION ...) from $CMAKELISTS" >&2; exit 1; }
SUFFIX=$(sed -n 's:.*set(TORS_VERSION_SUFFIX "\(.*\)").*:\1:p' "$CMAKELISTS")

CURRENT="$RELEASE"
[[ -n "$SUFFIX" ]] && CURRENT="$RELEASE-$SUFFIX"

usage() {
    echo "Usage: $0 <major|minor|patch|prerelease|X.Y.Z[-suffix]>" >&2
    echo "Current version: $CURRENT" >&2
    exit 1
}

[[ $# -eq 1 ]] || usage

IFS='.' read -r MAJOR MINOR PATCH <<< "$RELEASE"

case "$1" in
    major) NEW_RELEASE="$((MAJOR + 1)).0.0"; NEW_SUFFIX="" ;;
    minor) NEW_RELEASE="$MAJOR.$((MINOR + 1)).0"; NEW_SUFFIX="" ;;
    patch) NEW_RELEASE="$MAJOR.$MINOR.$((PATCH + 1))"; NEW_SUFFIX="" ;;
    prerelease)
        NEW_RELEASE="$RELEASE"
        if [[ "$SUFFIX" =~ ^([A-Za-z]+)\.([0-9]+)$ ]]; then
            NEW_SUFFIX="${BASH_REMATCH[1]}.$((${BASH_REMATCH[2]} + 1))"
        else
            NEW_SUFFIX="alpha.1"
        fi
        ;;
    [0-9]*.[0-9]*.[0-9]*)
        NEW_RELEASE="${1%%-*}"
        if [[ "$1" == *-* ]]; then
            NEW_SUFFIX="${1#*-}"
        else
            NEW_SUFFIX=""
        fi
        ;;
    *) usage ;;
esac

NEW="$NEW_RELEASE"
[[ -n "$NEW_SUFFIX" ]] && NEW="$NEW_RELEASE-$NEW_SUFFIX"

sed -i "s/project(TORS VERSION $RELEASE)/project(TORS VERSION $NEW_RELEASE)/" "$CMAKELISTS"
sed -i "s/set(TORS_VERSION_SUFFIX \"$SUFFIX\")/set(TORS_VERSION_SUFFIX \"$NEW_SUFFIX\")/" "$CMAKELISTS"

git add "$CMAKELISTS"
git commit -m "Bump version to $NEW"
git tag -a "v$NEW" -m "v$NEW"

echo "Bumped version: $CURRENT -> $NEW"
echo "Created commit and tag v$NEW (not pushed — run 'git push --follow-tags' when ready)"
