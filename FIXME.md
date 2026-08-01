# FIXME: `inStanding`/`outStanding` shape mismatch in `HIP_Scenario.proto`

**Status:** open, blocked on the generator/solver scenario-format unification
(no more separate `scenario_*.json` vs `scenario_solver_*.json`). Revisit
once that lands.

## Symptom

Parsing a real scenario JSON fails outright:

```
Parse JSON .../scenario.json / Status: INVALID_ARGUMENT:outStanding: Proto field is not repeating, cannot start list.
```

Reproduced via the C++ test suite:

```bash
export LOCATION_PATH="$(pwd)/data/Demo/TUSS-Instance-Generator/kleine_binckhorst"
export SCENARIO_PATH="$LOCATION_PATH/scenario.json"
export PLAN_PATH="$LOCATION_PATH/plan.json"
export RESULT_PATH=/tmp/result.txt
cd build && ctest --output-on-failure
# EngineTest and CompatibilityTest fail with the error above
```

## Root cause

`protos/HIP_Scenario.proto` declares `inStanding`/`outStanding` as
*singular* messages, each internally wrapping a repeated field:

```proto
message Scenario {
    ScenarioIn in = 1;
    ScenarioOut out = 2;
    ScenarioIn inStanding = 3;   // singular message
    ScenarioOut outStanding = 4; // singular message
    ...
}
message ScenarioIn  { repeated IncomingTrain trains = 1; }
message ScenarioOut { repeated TrainRequest trainRequests = 1; }
```

i.e. the JSON is expected to look like `"inStanding": {"trains": [...]}`.

But the real generator emits `inStanding`/`outStanding` as **bare top-level
JSON arrays** — confirmed both in this repo's own fixtures
(`data/Demo/TUSS-Instance-Generator/kleine_binckhorst/scenario.json`,
`.../in_out_standing_test/scenario.json`) and in
`../scenario-planning-inputs/Location_KleineBinckhorst/scenarios/scenario_*.json`
(the files `run_evaluator.py` actually feeds the evaluator with in the real
pipeline — see `../scenario-planning-inputs/run_evaluator.py:43`).

This matches `../robust-rail-generator/unified-schema-design.md`, which
documents the intended shape explicitly (around line 240-249):

```
inStanding:  list[IncomingTrain]
outStanding: list[TrainRequest]
```

So `HIP_Scenario.proto`'s wrapping in a singular `ScenarioIn`/`ScenarioOut`
message is simply wrong relative to both the documented design and the
actual wire format — `inStanding`/`outStanding` should be `repeated`, like
`in`/`out` are, not singular messages.

### It goes deeper than the wrapper

Even fixing the `repeated` vs singular mismatch isn't the whole story. The
actual entries in the *current production* `scenario_*.json` files (the
ones `run_evaluator.py` uses today) don't match `IncomingTrain`/
`TrainRequest`'s field names at all — they use the **old, non-HIP** field
names:

```json
{"id": "4002", "sideTrackPart": "56", "parkingTrackPart": "3",
 "members": [...], "canDepartFromAnyTrack": true,
 "standingIndex": 1.0, "minimumDuration": "60", "time": "0"}
```

vs. `IncomingTrain`'s HIP field names (`entryTrackPart`,
`firstParkingTrackPart`, `arrival`, `departure`, ...). There's a second,
already-HIP-shaped variant of these fixtures
(`scenario_solver_*.json`, e.g.
`../scenario-planning-inputs/Location_KleineBinckhorst/scenarios/scenario_solver_KleineBinckhorst_7t_custom_example1.json`)
where `inStanding`/`outStanding` *are* correctly wrapped
(`"inStanding": {"trains": [...]}`) — but that's not the file the real
pipeline currently points the evaluator at.

## Why this is blocked, not just fixed

There are two live scenario shapes in play right now (`scenario_*.json`,
old/non-HIP-named, currently what production actually feeds the evaluator;
`scenario_solver_*.json`, new/HIP-named). Which one the evaluator should be
reading is a cross-repo sequencing question, not something this repo can
decide unilaterally:

1. Should `inStanding`/`outStanding` become `repeated IncomingTrain`/
   `repeated TrainRequest` (per the design doc)?
2. Is the evaluator's real input about to switch from `scenario_*.json` to
   the unified/`_solver` shape, or does it need to keep reading the old
   shape for a while yet?

Once the generator/solver side finishes unifying on a single scenario
format (no more `scenario_*.json` vs `scenario_solver_*.json` split), fix
this by:

1. Changing `inStanding`/`outStanding` in `HIP_Scenario.proto` to
   `repeated IncomingTrain`/`repeated TrainRequest`.
2. Confirming the unified format's field names for standing trains actually
   match `IncomingTrain`/`TrainRequest` (rename/extend those messages if
   not — e.g. `canDepartFromAnyTrack` isn't currently a field on either).
3. Updating `cTORS/src/scenario/Scenario.cpp`'s
   `ImportShuntingUnits(const PB_HIP_Scenario &, ...)` (currently does
   `pb_scenario.instanding().trains()`, would become
   `pb_scenario.instanding()` directly) and
   `ImportAncillary`/related call sites accordingly.
4. Updating this repo's own fixtures
   (`data/Demo/TUSS-Instance-Generator/kleine_binckhorst/scenario.json` and
   `.../in_out_standing_test/scenario.json`) to match.
5. Re-running `EngineTest`/`CompatibilityTest` to confirm the parse error
   is gone.

## Related

- `protos/HIP_Scenario.proto` (`inStanding`/`outStanding` fields, added in
  `989806c`)
- `cTORS/src/scenario/Scenario.cpp` (`ImportShuntingUnits`, two overloads —
  old `PBScenario` one already treats `instanding()`/`outstanding()` as
  repeated directly, confirming the HIP overload's singular-wrapper shape
  is the outlier)
- `../robust-rail-generator/unified-schema-design.md` (documents the
  intended `list[IncomingTrain]`/`list[TrainRequest]` shape, and separately
  flags `canDepartFromAnyTrack` as an open question for `TrainRequest`)
- `../scenario-planning-inputs/run_evaluator.py` (what the real pipeline
  currently feeds the evaluator)
