# FIXME: env-var-gated EngineTest/CompatibilityTest cases can't run without manual setup

**Status:** open, deferred. Not blocking anything - these are the two `ctest` failures
that have been present throughout the scenario-unification work (`noproto` branch) and
are expected/known, not regressions. Revisit when there's appetite to build proper
committed fixtures for them.

## Symptom

`ctest` always reports exactly these two failures, regardless of other work on the
branch:

```
The following tests FAILED:
      4 - EngineTest (Failed)
      5 - CompatibilityTest (Failed)
```

Specifically, in each of those binaries, the *first* `TEST_CASE` fails on a `REQUIRE`,
and the *second* fails as a downstream consequence:

- `cTORSTest/EngineTest.cpp`: `"Scenario and Location test"` then `"Plan test"`
- `cTORSTest/CompatibilityTest.cpp`: `"Scenario and Location Compatibility test"` then
  `"Plan Compatibility test"`

## Root cause

Each file declares file-scope string globals (`location_path`, `scenario_path`,
`plan_path`, and `result_path` in `CompatibilityTest.cpp`), populated only inside the
*first* `TEST_CASE` from `getenv("LOCATION_PATH")` / `SCENARIO_PATH` / `PLAN_PATH` /
`RESULT_PATH`. Nothing in this environment exports those variables, so:

1. The first test case hits `REQUIRE(LOCATION_PATH != nullptr)` immediately and
   aborts - the globals never get assigned, staying as empty strings.
2. The second test case doesn't re-check env vars at all - it uses the still-empty
   `location_path` directly: `LocationEngine engine(location_path)` tries to open
   `"" + "/location.json"`, which doesn't exist, and throws.

One root cause (missing env vars), cascading into two failures per file. This predates
the scenario-unification work entirely.

## A new wrinkle from the scenario-unification work

`LocationEngine::GetScenario(scenario_path)` -> `Scenario(string, const Location&)` now
parses *exclusively* through the unified HIP schema (no legacy fallback - see
`cTORS/src/scenario/Scenario.cpp`). This means even if someone exports the four env
vars today, pointing `SCENARIO_PATH` at any of the repo's pre-existing local fixtures
won't work:

- `data/Demo/TUSS-Instance-Generator/kleine_binckhorst/scenario.json` - old field names
  (`sideTrackPart`, `time`, ...), predates the unification entirely.
- `.../scenario_solver.json` - HIP-shaped, but stale relative to the flattening
  (`in`/`out`/`inStanding`/`outStanding` are wrapped, not flat arrays) and the
  `typePrefix`+`carriages` identity fix (still embeds a full `type` object per
  `TrainUnit`).

The only fixture currently matching the finalized unified schema is
`cTORSTest/fixtures/scenario_unification_test/` (added for `EngineTest`'s
`"Scenario unification"` case), and it has no matching `Plan.json` - so even
exporting env vars wouldn't get a fully green manual run today without also
authoring a compatible Plan file.

## What it would take to actually fix this

Two different things could be meant by "fix," worth separating:

1. **Make manual runs possible again**: regenerate (or hand-author) a `scenario.json` +
   `scenario_solver.json`-equivalent pair in the unified shape for one of the existing
   demo locations, plus a matching `plan.json`. Lower effort, but only helps people who
   remember to export the env vars - doesn't add CI coverage.
2. **Make these run in default `ctest`, self-contained** (the more valuable fix): build
   small committed fixtures and rewrite the test bodies to not depend on env vars, the
   way `EngineTest`'s `"Scenario unification"` case already does. Two sub-parts with
   different scope:
   - `EngineTest.cpp`'s `"Plan test"` exercises the *legacy* Run round-trip
     (`GetRunResultProto` / old-proto path via `RunResult::CreateRunResult(&location,
     const PBRun&)`) - unrelated to the HIP schema, needs its own fixture: a location +
     a `Run.json` with an embedded legacy-shaped scenario and plan.
   - `CompatibilityTest.cpp`'s two cases exercise the HIP scenario path plus the HIP
     Plan-reading path (`ParseHIP_PlanFromJson` / `RunResult::CreateRunResult(const
     PB_HIP_Plan&, ...)`) - closer to what's actually valuable to have running in CI,
     but needs a full matching `Plan.json` alongside Scenario + Location, which
     doesn't exist yet for the synthetic `scenario_unification_test` fixture.

## Related

- `cTORSTest/EngineTest.cpp`, `cTORSTest/CompatibilityTest.cpp`
- `cTORSTest/fixtures/scenario_unification_test/` (the one fixture that does match the current
  unified schema, but is Scenario+Location only, no Plan)
- `cTORS/src/scenario/Scenario.cpp` (`Scenario(string, const Location&)` - the
  HIP-only string-path constructor)
- `cTORS/src/engine/Plan.cpp` (`GetRunResultProto`, `ParseHIP_PlanFromJson`,
  `RunResult::CreateRunResult` - both the legacy and HIP variants)
