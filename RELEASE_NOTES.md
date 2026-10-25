# Release notes

## 2.3.0 — 2026-10-25

### Exact integer-millimetre lengths (#39)

Track and train-unit lengths were compared as raw `double`s, so a layout that
fills a track exactly (e.g. three 24.1 m units on a 72.3 m track) could be
rejected by floating-point noise. Lengths are now stored as integer
millimetres (`llround(metres * 1000)`, half away from zero) and compared
exactly with a strict `>`, matching the solver. The public API (protobuf,
JSON, constructors, `GetLength()`, pyTORS `length`) stays in metres. A length
with more than 3 decimals is rounded and a warning is printed to stderr.

Verdicts can change: exact fits that previously failed by one ulp now pass.

### Facility capacity counts shunting units (#38)

`capacity_facility_rule` counted every active `ServiceAction` against a
facility's capacity. A unit with several members gets one service action per
member, so a two-member unit used up a capacity-2 facility on its own, and the
rule also let a third unit in (it only compared the count after the loop,
and not against the unit being validated). The rule now counts distinct
shunting units, including the unit of the action under validation, and
compares once against the capacity. A facility with capacity 0 refuses every
service.

Verdicts can change in both directions: plans that used a facility with
several members at once and were wrongly rejected now pass, and plans that
over-filled a facility and were wrongly accepted now fail.

### Clearer errors for degenerate Move and Setback paths (#30, #31)

- A HIP `Move` with no resources (a unit "moving" to the track it is already
  on, as the solver emits) produced a one-entry path, and
  `MoveAction::GetPreviousTrack()` read out of bounds on it, which could
  segfault later. Such a `Move` is now skipped during plan conversion, and
  `GetPreviousTrack()` throws an `InvalidActionException` for a path with
  fewer than two entries.
- `Track::GetNextTrackParts` (and `GetOppositeSide`) threw an opaque
  `std::out_of_range` when `previous` was not a real neighbour of the track,
  e.g. a `Setback` of an `inStanding` train before its first `Move`. It now
  throws an `InvalidActionException` naming the tracks involved.
- Skipping a no-op `Move` (zero duration or no resources) during HIP plan
  conversion now prints a warning instead of happening silently.

### A Move may not drive through a unit parked on its first track (#20)

`blocked_track_rule` is meant to reject a multi-track `Move` whose path
crosses a track occupied by another shunting unit. It skipped the first track
the unit enters, as well as the destination, so a unit parked on that first
track could be driven straight through. The unit's own starting track is not
part of the checked list, so only the destination is a legitimate exception
(the unit stops there, and `length_track_rule` decides whether it fits). The
first entered track is now checked like every other track the unit passes.

Verdicts can change: plans that drove through a unit parked on the first track
of a Move are now rejected. On the two locations shipped with the pipeline
(Kleine Binckhorst and SimpleService) this cannot happen: the first track a
Move enters there is always a zero-length switch, which cannot hold a parked
unit, so no verdict on those locations changes. The fix matters for locations
where parkable tracks are directly adjacent. It is covered by a rule-level unit
test; no real plan exercises it.

### `--departure_delay` is applied per scenario

The `--departure_delay` option was parsed but not copied into the `State`, so
the exit-time rule never saw it. It now allows a departing unit to leave up to
that many seconds before or after its scheduled time (default 0: exact match
required). `outStanding` units ignore it.

Verdicts can change: runs that pass a non-zero `--departure_delay` now accept
exits inside that window.

### Build requirements

Ubuntu 22.04 is no longer supported: its protobuf 3.12 cannot compile the
`optional` fields in `protos/`. The minimum CMake version is now 3.28 (what
Ubuntu 24.04 ships), pyTORS uses pybind11 v3.1.0, and the unit tests use
doctest v2.5.1 (v2.4.11 does not configure on CMake 4).

### Docker image

The image build no longer clones dependencies from GitHub inside the main
build step, which failed intermittently: `json` and `pybind11` are cloned in
a separate, cached stage (`docker-fetch-deps.sh`), and the unit tests (and so
doctest) are no longer built in the image.

### CI

The test workflow now runs on `ubuntu-24.04`, `ubuntu-24.04-arm` and
`ubuntu-26.04`, with a dependency cache per runner.

## 2.2.0 — 2026-09-26

Adopts interchange **schemaVersion 2** (see `SCHEMA_CHANGELOG.md` in
`robust-rail-general`): an explicit `Reverse` action for an in-place reversal,
and optional feasibility/origin/cost metadata on a `Plan`. Plans declaring
`schemaVersion` 1, or none at all, still evaluate exactly as before.

### Explicit reversals: the `Reverse` task type (#13)

A saw movement (drive onto a track, reverse, drive back out) used to be
embedded in a single `Move` action, and the evaluator's front/back
orientation tracking went stale across it — the train could end up flipped
in an unphysical way. Reported with a full repro by
[@luteberget](https://github.com/luteberget) — thanks.

- The `PredefinedTaskType` value `Walking` (6) is renamed to `Reverse`.
  Nothing emitted `Walking` as a real value.
- A Solver-format plan's `Reverse` task now becomes a real reversal action.
  Previously the HIP-format conversion had no case for it and dropped the
  task silently.
- A `Move` that ends right before its unit's `Reverse` no longer gets a
  spurious `EndMove`, which the parking rules used to reject on a
  non-parking track such as a gateway.
- A `Move` that embeds a saw movement:
  - under `schemaVersion` 1 or none: still accepted, with a deprecation
    warning;
  - under `schemaVersion` 2: rejected, with a message asking for an explicit
    `Reverse` action.
- `EXPECTED_SCHEMA_VERSION` is now 2. A plan declaring 1 gets the usual
  warn-and-continue mismatch warning, not a rejection.

The evaluator's internal names (`SetbackAction`, the `setback_*` business
rules and their `config.json` keys) are unchanged.

### Plan feasibility, origin and cost

A `Plan` may now carry `feasibility` (`Unknown`/`Feasible`/`Infeasible`),
`origin` (free text identifying what produced the plan), `cost`, and
`costDetails`. All are optional. The evaluator parses them, and logs a
warning (never a rejection) when a plan's declared feasibility disagrees with
the evaluation result. `Unknown` or absent is never checked.

### Fixes

- **Replayed multi-hop Moves use the plan's own duration (#18).** The
  evaluator used to recompute a replayed `Move`'s duration from a fixed
  per-track-type sum. On a long enough route, the move was then still
  "active" when the plan's next action for that unit was due, which failed
  with a misleading "shunting unit is already active" error.
- **Service tasks on coupled shunting units (#25, #26).** A train with no
  declared task crashed the evaluator with `unordered_map::at` and exit code
  0. It now gets the existing "Could not find task" rejection. Coupled
  members that share a service task are now serviced sequentially, matching
  the solver's model, instead of in parallel.
- **Per-type train counts in scenario checks (#24).** Each per-type count
  in `CheckScenarioCorrectness` was capped at 1. The related error message
  is also cleaned up. Thanks to
  [@MayteSteeghs](https://github.com/MayteSteeghs).
- **Exit-mismatch diagnostic (#16).** When an `Exit` action matched no
  departure, the error listed every outgoing train. It now lists only trains
  whose IDs actually match.
- **Unknown task types are now rejected, not silently dropped (#28).** A plan
  action whose task type wasn't a known `PredefinedTaskType` name (for
  example `"Setback"` after its rename to `"Reverse"`) used to parse with the
  action silently dropped from evaluation — no error, no trace, plan looked
  cleanly evaluated with a piece missing. Now throws, naming the action's
  index, start time and shunting unit. A known predefined type with no
  conversion case (`Break`, `NonService`) is caught the same way.
  `ignore_unknown_fields` otherwise stays on: unknown *fields* are still
  tolerated by design.

### Known limitations

- The reversal duration comes from the train type, not the plan. A plan
  that schedules a longer `Reverse` is fine: the unit finishes early and
  waits.

### Build and repo hygiene

- `build.sh` is now the one build entry point, used by the Dockerfile, the
  devcontainer and the VS Code tasks. It builds in parallel under `nice`.
- The Docker build uses a persistent ccache mount and registry build
  caching.
- `docker-push.sh`/`docker-push-edge.sh` work from any directory, and log
  out of ghcr.io when they exit.
- CI fails on duplicate doctest `TEST_CASE` names, and a duplicated pair in
  `CompatibilityTest.cpp` is removed.

### Publishing

Unchanged: versioned from `CMakeLists.txt`'s `project(TORS VERSION ...)`,
pushed to `ghcr.io/robust-rail-nl/tors` via `docker-push.sh`.

## 2.1.0 — 2026-09-11

A fix-focused release. No interchange-format changes — this is not a
lockstep release with generator/solver. Does **not** include the
saw-movement/orientation fix for evaluator#13 — that lands its own
explicit-`Setback` wire change and is being rolled out via the `edge`
channel first, alongside matching solver/planner changes; see
[#13](https://github.com/Robust-Rail-NL/robust-rail-evaluator/issues/13).

### Two of 2.0.0's known limitations are fixed upstream

Both fixtures called out in 2.0.0 as expected-to-fail now produce valid
plans, fixed on the solver side (solver 2.1.0):

- **solver#13** — a delayed Arrival now carries a real duration instead of a
  trailing Wait, fixing `6t_custom_example3`.
- **solver#14** — outStanding trains are now costed against the scenario end
  time, so the solver no longer produces a plan that overruns the horizon for
  free. `7t_custom_example1` now evaluates cleanly. Note: this avoids
  triggering [evaluator#6](https://github.com/Robust-Rail-NL/robust-rail-evaluator/issues/6)
  rather than fixing it — the underlying diagnostic-quality bug
  (`EvaluatePlan` reporting the symptom, not the cause, for a plan that
  genuinely does extend past the horizon) is still open.

solver#18 (ignored `standingIndex`) is also fixed upstream; TORS itself
already honoured it, so nothing changes here.

### Fixed: scenario task-duration check summed across the wrong scope (#12)

`Scenario::CheckScenarioCorrectness` rejected a scenario if the *sum* of
every train's required task duration exceeded the scenario's end time —
which wrongly rejects a scenario where that time is spread across multiple
facilities in parallel (e.g. two hours of work split across two maintenance
facilities in one hour). The check is now scoped per shunting unit: a single
unit's own tasks still can't exceed the scenario end time, but different
units no longer share one global budget. Reported with a full repro by an
external user — thanks to [@luteberget](https://github.com/luteberget).

### JSON input validation (closes evaluator#1)

Several gaps where malformed or wrong-shaped JSON produced a silent
default/empty result instead of an error are closed:

- A `Location`/`Scenario`/`Plan`/`Run` JSON document with some unrelated
  field set but none of its essential content (e.g. a location with no
  `trackParts`) now fails at the parse boundary with a clear message,
  instead of loading "successfully" and crashing several steps later with
  an unhelpful `map::at`.
- Syntactically valid JSON that doesn't match the target message shape at
  all now throws, instead of silently parsing into an all-default,
  empty message.
- `main.cpp` catches and reports these failures on every `--plan_type`
  branch (previously only some paths had a `try`/`catch` at all).
- `--plan_type` is now validated and defaults to `Solver` (previously
  accepted any free-form string, silently doing nothing on a typo).

### Repo hygiene

- Added the `edge` image channel (`docker-push-edge.sh`) alongside the
  existing `stable` release path — see `CONTRIBUTING.md`. `edge` builds now
  also report their own `<version>-edge+<date>.<sha>` string at startup,
  matching what's already in the image's OCI label.
- Retired the unmaintained Python front-end (`TORS/`) — a strictly older,
  never-shipped snapshot of the same code developed in `AlgTUDelft/cTORS`.
  `pyTORS` (the pybind11 binding) is kept; only the application built on top
  of it is gone.
- Removed orphaned files with no live consumer: `.gitlab-ci.yml`,
  `setup.py`, `vis_config.json`.

### Publishing

Unchanged from 2.0.0: versioned from `CMakeLists.txt`'s `project(TORS
VERSION ...)`, pushed to `ghcr.io/robust-rail-nl/tors` via `docker-push.sh`.

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
- **`canDepartFromAnyTrack` is dropped from the wire format entirely.**
  TORS's own serializer (`TrainGoal::Serialize`) hardcoded it to `false` on
  output regardless of input and never read it back for any decision; the
  generator only ever wrote it; the solver's only former consumer of it
  (`Converter.cs`) was already gone. Zero behavioral effect, no code here
  changes what it does — see generator's `SCHEMA_CHANGELOG.md`
  ("Unversioned — 2026-08-21") for the full trace.

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
  under `cTORSTest/fixtures/`, rather than reading paths from environment
  variables that nothing in CI ever set.
- Added a CI workflow (`ctest.yml`) that builds and runs the full test suite
  on push/PR against `main`, `dev` and `release/2.0.0`.
- Exceptions raised while loading config/location/scenario files are no
  longer silently sliced to `std::exception`, so load failures report their
  actual cause.
- The repo's one demo location (`example_kleine_binckhorst/`, referenced by
  the README, `run_eval_example.sh`/`run_eval_and_store_example.sh`, and the
  Docker image) predated the schema unification and threw `map::at` at load
  time under the now-HIP-only scenario parser. Replaced its data with the
  already-migrated, already plan-valid Kleine Binckhorst fixture that
  `CompatibilityTest` exercises, and pointed that test at
  `example_kleine_binckhorst/` directly instead of a duplicate copy under
  `cTORSTest/fixtures/`, so the two consumers now share exactly one copy of
  the data.

### Publishing

The TORS image is versioned from `CMakeLists.txt`'s `project(TORS VERSION
...)` plus `TORS_VERSION_SUFFIX` and pushed to `ghcr.io/robust-rail-nl/tors`
via `./docker-push.sh` (multi-arch: `linux/amd64`, `linux/arm64`), alongside a
`-assert` image built with `-DCTORS_ASSERTIONS=ON` for the pipeline's
assertion-enabled evaluation pass. The `tors:2.0.0` tag points at the same
image digest already verified as `2.0.0-rc.3` — re-tagged, not rebuilt, so
the tag names exactly the bytes that were tested.
`:latest` moves to `2.0.0` as the first stable tag of the release; it does not
move for `-rc.*`/`-beta.*` builds.
