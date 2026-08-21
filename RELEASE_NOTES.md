# Release notes

## 2.0.0 — 2026-08-20

This is the evaluator's (TORS's) slice of the shared 2.0.0 release: the same
interchange format, tagged together, across `robust-rail-generator`,
`robust-rail-solver` (HIP) and `robust-rail-evaluator` (TORS). The full
cross-repo picture — verification evidence, what each repo changed, and the
decisions behind the schema — lives in `scenario-planning-inputs`'
`docs/roadmap-2.0.0.md`.

### Binary protobuf is gone; the wire format is JSON

TORS still uses generated protobuf message classes internally, but only as an
in-memory representation reached via `JsonStringToMessage` — no binary
protobuf (de)serialization exists anywhere in this repo any more.
`Location`, `Scenario` and `Plan` are read directly from the JSON interchange
format on the HIP path, which is now the only path exercised by the pipeline.

### Interchange format: breaking changes from the pre-2.0.0 shape

Anything feeding `location.json`/`scenario.json`/plan JSON to TORS against the
pre-migration shape needs to account for:

- **`displayName` → `typePrefix` + `carriages`.** A train unit type is
  identified by the pair, keyed via `typesByPrefixAndCarriages`, not a
  combined string like `"SLT4"`. Includes a fix for carriage-blind type
  matching that the old string-keyed lookup was masking.
- **`TaskSpec.priority` (int) → `TaskSpec.optional` (bool).** TORS only ever
  read `priority` as a zero/non-zero flag.
- **`Resource` is `{ "kind": "trackPart"|"facility"|"staff", "id": <int> }`.**
  Fixed a wire-shape mismatch here that was crashing Track/Facility resource
  handling on the HIP path.
- **Every ID is an `int`**, including `TrainUnit.id` (was string, with a
  `"****"`/`stoi()` sentinel for "unassigned" — gone) and every composite id,
  now read directly as numbers instead of parsed back out of strings.
  `Plan.Action.shuntingUnit` references train units by ID (`memberIDs`) rather
  than embedding them.
- **`Plan.trackParts` is dropped.** Infrastructure comes from
  `--path_location`, as it already did; this field was dead weight on the
  wire.
- **Enum wire values are PascalCase** (`"StandIn"`, not `"standIn"`); the
  `allow_alias`/lowercase aliases on `PredefinedTaskType` are gone.
- **`standingIndex` is `int32`**, not `double`. Nothing read the magnitude,
  only compared it to establish order, and `TrainGoal` already truncated it to
  `int` internally — the wire type now matches what was already true
  in-process. TORS honours the field (`TrainGoal::standingIndex`, solver#18 is
  a solver-side gap, not an evaluator one). Fixture `standingIndex` values
  that were purely decorative (every track in the corpus has exactly one
  standing unit) were nulled out so they don't imply an ordering nothing
  enforces.
- **`Scenario`'s `in`/`out`/`inStanding`/`outStanding` are flat arrays**, not
  wrapped or nested under a `trainUnit` object.

`schemaVersion` is parsed from `Location`, `Scenario` and `Plan` JSON; a
mismatch is a logged warning, never a hard reject.

### Replay-mode fixes

TORS's business rules were originally written for its own search mode and
inherited unchanged into plan replay, where several had the wrong contract:

- Movements ending on a non-parking track are no longer rejected —
  `legal_on_parking_track_rule` exempts step-by-step search moves already, but
  every replayed departure's last movement lands on the gateway, which is a
  non-parking track by construction.
- Replaying a `Wait` action now honours the plan's own supplied duration
  instead of running until the next queued event (correct for search, which
  has no plan and re-decides at every event; wrong for replay).
- `EndMove` suppression now looks ahead to the shunting unit's own next action
  rather than any unit's, and coinciding-Wait-action merging was reviewed
  alongside it.
- `Combine` post-processing no longer leaves the first unit in both operand
  lists.
- outStanding shunting units can exit at the end of the scenario.
- `EvaluatePlan` has a safety valve for the case where the plan still has
  actions but the state is terminal, so a stalled replay terminates instead of
  hanging (evaluator#6 below is the follow-up: the valve stops the hang but
  the diagnostic it reports is the symptom, not the cause).

### Known limitation: two canonical fixtures can't produce a valid plan

`6t_custom_example3` and `7t_custom_example1` are expected to fail, not from
anything in this release, but from open issues upstream:

- `6t_custom_example3`: the solver parks on a non-parking arrival track when
  it can't move into the yard immediately (solver#13).
- `7t_custom_example1`: the solver's cost function has no deadline for
  outStanding trains, so it produces a plan that overruns the scenario
  horizon for free, which trips
  [evaluator#6](https://github.com/Robust-Rail-NL/robust-rail-evaluator/issues/6):
  `EvaluatePlan` spins when the plan still has actions but the state is
  terminal, reporting the symptom rather than "plan extends past the
  horizon." The safety valve above terminates it; the diagnostic is still
  misleading.

Both were deferred deliberately rather than blocking 2.0.0.

### One more schema-adjacent change after the initial cut

`canDepartFromAnyTrack` is dropped from the wire format entirely, landed
after this file was first written. TORS's own serializer
(`TrainGoal::Serialize`) hardcoded it to `false` on output regardless of
input and never read it back for any decision; the generator only ever
wrote it; the solver's only former consumer of it (`Converter.cs`) was
already gone. Zero behavioral effect, no code here changes what it does —
see generator's `SCHEMA_CHANGELOG.md` ("Unversioned — 2026-08-21") for the
full trace.

### Other known issues, not blocking

- [**evaluator#1**](https://github.com/Robust-Rail-NL/robust-rail-evaluator/issues/1)
  — invalid JSON fails quietly, but only on the legacy `--plan_type Evaluator`
  path, which the pipeline never uses (`run_evaluator.py` always passes
  `Solver`). Retiring that path entirely is an open decision in
  `roadmap-2.0.0.md`, not resolved by this release.
- **solver#17** — solver and evaluator place a combined inStanding train's
  members at opposite ends of the track, so the solver can route a departing
  half out of the blocked end and call the result feasible. Needs a decision
  on which convention is right; a companion evaluator issue hasn't been filed
  yet, since no fixture in this corpus exercises it (a combined inStanding
  train that gets split).
- **solver#18** — the solver ignores `standingIndex`; TORS itself already
  honours it (see above), so this is purely a solver-side gap. Latent in this
  corpus: every scenario leaves the field null.

### Repo hygiene

- CI now builds and tests on `ubuntu-24.04-arm` as well as `ubuntu-latest` —
  several developers work on arm64, and this is the C++ component where an
  architecture difference would plausibly show up. `EngineTest` and
  `CompatibilityTest` were made self-contained against committed fixtures
  under `data/`, rather than reading paths from environment variables that
  nothing in CI ever set.
- Added a CI workflow (`ctest.yml`) that builds and runs the full test suite
  on push/PR against `main`, `dev` and `release/2.0.0`.
- Exceptions raised while loading config/location/scenario files are no
  longer silently sliced to `std::exception`, so load failures report their
  actual cause.

### Publishing

The TORS image is versioned from `CMakeLists.txt`'s `project(TORS VERSION
...)` plus `TORS_VERSION_SUFFIX` and pushed to `ghcr.io/robust-rail-nl/tors`
via `./docker-push.sh` (multi-arch: `linux/amd64`, `linux/arm64`), alongside a
`-assert` image built with `-DCTORS_ASSERTIONS=ON` for the pipeline's
assertion-enabled evaluation pass. The `tors:2.0.0` tag points at the same
image digest already verified as `2.0.0-rc.2`
(`sha256:d560541a6e2f9d5e58ec0d8bbb0aad180109b804dd2b9bd6e235cdb59b97ee5b`) —
re-tagged, not rebuilt, so the tag names exactly the bytes that were tested.
`:latest` moves to `2.0.0` as the first stable tag of the release; it does not
move for `-rc.*`/`-beta.*` builds.
