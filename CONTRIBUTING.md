# Contributing

## Image channels: `stable` and `edge`

The TORS evaluator image is published under two channels:

- **`stable`** — the existing reviewed-release path. A tagged `X.Y.Z` (or
  prerelease `X.Y.Z-suffix`) image, built and pushed via
  [`docker-push.sh`](docker-push.sh) once a change has gone through its own
  PR and been merged into `main`. `tors:latest` always means "last real
  release."
- **`edge`** — a fast, lower-ceremony channel for running a not-yet-reviewed
  fix without waiting for its PR into `main`. Built and pushed via
  [`docker-push-edge.sh`](docker-push-edge.sh) under the floating `tors:edge`
  tag, always overwritten by the newest push to the `edge` branch.

### Branch flow

- **Every change still goes through its own feature branch and PR into
  `main`.** `edge` does not replace that — it runs alongside it. Never
  commit directly to `edge`.
- **To get a fix onto `edge` early**, merge its feature branch into `edge`
  (`git merge --no-ff <branch>`) in addition to opening the normal PR into
  `main`. Once that PR is reviewed and merged, `edge` already has the
  content — nothing needs to be cherry-picked or re-applied.
- **`edge` only ever advances via merge commits** (`git merge --no-ff`),
  whether merging in a feature branch or catching up with `main`. Never
  rebase `edge`, and never fast-forward it — the point is for its history to
  show what was merged in and when, as a readable sequence of merge commits,
  not a flattened line that hides which branch each change came from.
- **Flow between `edge` and `main` is one-directional**: `main → edge`
  only, via periodic `git merge --no-ff main`. Nothing flows from `edge`
  back into `main` directly, since nothing should ever exist on `edge` that
  doesn't also exist on some reviewed feature branch (see above).
- One accepted gap: if `edge` is ever force-pushed or rebased despite the
  above, a dropped commit won't show up in `git log` — only `git reflog` or
  GitHub's Actions run history would have it. Treated as a reasonable
  tradeoff for a fast-moving channel; revisit only if that actually causes
  a real problem.

### Publishing `edge`

Currently manual only — run [`docker-push-edge.sh`](docker-push-edge.sh)
yourself from the `edge` branch (it checks and refuses to run from anywhere
else). There is no CI automation building or pushing `edge` on every push to
the branch. This was a deliberate choice, not an oversight: wiring that up
means a new category of CI workflow (deploy, not just build/test) and
storing GHCR push credentials as Actions secrets — a real increase in attack
surface for a channel whose whole point is to move fast with less
ceremony. With a single publisher and no other consumers yet, the manual
script gets the speed benefit (skipping *review*, not the human decision to
publish) without that cost. Revisit once there's an actual need — more
people needing to publish, or the manual step becoming the actual
bottleneck.

### Version string

`edge` images don't reuse `CMakeLists.txt`'s `project(TORS VERSION ...)` /
`TORS_VERSION_SUFFIX` as-is (that's what distinguishes a `stable` build).
Instead: `<release>-edge+<date>.<short-sha>`, e.g.
`2.0.0-edge+20260904.a1b2c3d`.

- `edge` is its own semver prerelease identifier, not chained onto whatever
  prerelease suffix (if any) `TORS_VERSION_SUFFIX` currently holds — the
  release portion is always the bare `X.Y.Z` from `project(TORS VERSION
  ...)`.
- Date + short commit SHA are semver **build metadata** (after the `+`),
  not part of the prerelease identifier — build metadata doesn't affect
  version precedence/sorting, which is correct here: an edge build should
  never be compared as if it were an ordered release, but a human or a bug
  report should still be able to trace a running image back to an exact
  commit and day.
- No git tag is created per edge build (unlike `bump-version.sh`'s tags for
  real releases) — since every push to `edge` is meant to become a new
  image, the branch history itself is the record of what produced what.

### No `-assert` or `:devel` variant for `edge`

`docker-push.sh` publishes a `$VERSION-assert` companion to every release
(`-DCTORS_ASSERTIONS=ON`, consumed by robust-rail-general's pipeline, which
evaluates every plan under both the plain and `-assert` image) and a
floating `:devel` tag (the builder stage on its own, for
`.devcontainer/devcontainer.json`). `docker-push-edge.sh` publishes neither.
Keeping edge to a single image keeps the channel simple and matches the
solver repo's edge, which also skips its `-assert` equivalent. Revisit if an
edge-assert companion turns out to be wanted — e.g. to exercise
robust-rail-general's dual-evaluation flow against an unreviewed fix.

### Test workflow on `edge`

`edge` is listed under `push` (not `pull_request`) in
[`.github/workflows/ctest.yml`](.github/workflows/ctest.yml), since it only
ever gains work via direct `git merge --no-ff`, never a GitHub PR targeting
it.
