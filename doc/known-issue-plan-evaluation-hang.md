# Known issue: `LocationEngine::EvaluatePlan` can hang forever on certain plans

**Status:** open, root cause localized but not pinned down. Not caused by the
protobuf-compat patch (`noproto` branch, commits `55e206b`..`e10061f`) — see
"Why this isn't a regression" below.

## Symptom

`TORS --mode EVAL_AND_STORE ... --plan_type Solver` never terminates. stdout
repeats the same block forever without the simulated time ever advancing:

```
Execute immediate events (3 events queued)
Next event at T=1900: Incoming train
Step done.
Execute immediate events (3 events queued)
Next event at T=1900: Incoming train
Step done.
...
```

**⚠️ Operational warning:** each iteration writes to both stdout and the
`--path_eval_result` file. Left unbounded this produces gigabytes of output
in well under a minute — it filled a 217GB disk to 100% during the
investigation that found this issue. **Always wrap reproduction attempts in
`timeout` and cap captured output** (e.g. `| tail -c 5000`), never redirect
to a plain file without a timeout.

## Confirmed reproductions

Both against locally-built `TORS` (built from this repo) driving the fixture
pairs in the sibling `../scenario-planning-inputs` repo:

- `Location_SimpleService`: `scenarios-pydantic/scenario_simple_service_location_4t_custom_late.json`
  + `plans-pydantic/plan_simple_service_location_4t_custom_late.json`
- `Location_KleineBinckhorst` (current/pydantic `scenarios/` + `plans/`,
  not the `-protobuf` variants): `plan_KleineBinckhorst_6t_custom_example3`,
  `_7t_custom_example1`, `_8t_custom_example2` all hang; `_30t_random_98s_test`
  completes fine; `_10t_random_42s_distribution{1,2}` fail cleanly with a
  legitimate business-rule rejection (train length exceeds track length) —
  no hang.

Reproduction command shape (adjust paths/names):

```bash
cd ../scenario-planning-inputs/Location_SimpleService
timeout 10 /path/to/robust-rail-evaluator/build/TORS \
  --mode EVAL_AND_STORE \
  --path_location "$(pwd)" \
  --path_scenario "$(pwd)/scenarios-pydantic/scenario_simple_service_location_4t_custom_late.json" \
  --path_plan "$(pwd)/plans-pydantic/plan_simple_service_location_4t_custom_late.json" \
  --path_eval_result /tmp/eval.txt \
  --plan_type Solver 2>&1 | stdbuf -o0 tail -c 3000
```

To get a stack trace instead of just watching it spin, run under `gdb` in the
background and send `SIGINT` after a few seconds:

```bash
timeout 6 gdb -batch \
  -ex "set pagination off" \
  -ex "run --mode EVAL_AND_STORE --path_location ... --path_scenario ... --path_plan ... --path_eval_result /tmp/eval2.txt --plan_type Solver > /dev/null 2>&1" \
  -ex "interrupt" -ex "bt" \
  --args /path/to/build/TORS > /tmp/gdb_hang.txt 2>&1 &
BGPID=$!
sleep 4
kill -INT $BGPID
wait $BGPID
```

## Why this isn't a regression from the compat patch

Built the actual `v1.3.0` release tag in an isolated git worktree
(`git worktree add --detach <dir> v1.3.0`) and ran it against the exact same
pydantic-format fixture. It does **not** hang — it fails to parse the input
at all under v1.3.0's strict (non-`ignore_unknown_fields`) JSON parsing,
prints `Protobuf is empty or not initialized`, evaluates an empty scenario
(0 arrivals, 0 departures), and trivially reports "The plan is valid".

So the hang was never *reachable* before this session's compat patch — the
old code rejected the new-format input outright before it ever got deep
enough into the simulation to hit whatever is wrong. The compat patch
(tolerant JSON parsing + `memberIDs` support) is what lets real plan data
flow far enough to expose a pre-existing latent bug in the engine's event
loop. The bug itself predates this session; only its *visibility* is new.

## Where it's localized

`cTORS/src/engine/Engine.cpp`:

- `LocationEngine::EvaluatePlan` (two overloads, ~line 365 and ~437): the
  outer `while (it != plan.GetActions().end())` loop. On each iteration,
  if `state->GetTime() >= it->GetSuggestedStart()` it applies the current
  planned action and advances `it`; otherwise it calls
  `ApplyWaitAllUntil(state, it->GetSuggestedStart())`.
- `ApplyWaitAllUntil` (~line 235): iterates **all** shunting units in
  `state->GetShuntingUnitStates()`. For each one, only applies a
  `WaitAction` if `!suState.waiting && time - state->GetTime() > 0 &&
  !suState.HasActiveAction()`. Unconditionally calls `Step(state)`
  afterward, whether or not it applied any wait.
- `Step` (~line 55): `while (!state->IsActionRequired() &&
  state->GetNumberOfEvents() > 0)` pops and executes the next queued event.
  Critically, **if `IsActionRequired()` is already true, this loop body
  never runs at all** — no event gets popped, no time passes.
- `State::IsActionRequired()` (`cTORS/src/state/State.cpp` ~line 285):
  true if `IsAnyInactive()` (some shunting unit is idle — not moving, not
  waiting, no active action) or if some incoming train's arrival time
  equals the current time.

**Working hypothesis:** a shunting unit becomes idle (`IsAnyInactive()` ==
true) at a point where the plan's action iterator `it` is not yet pointing
at an action for *that* shunting unit — it's pointing at a chronologically
later action belonging to a different shunting unit (e.g. the second
train's arrival at T=1900). `ApplyWaitAllUntil`'s per-shunting-unit
skip-condition means it never issues a wait for the idle unit (or for
anyone), `Step()` refuses to pop the queued event because
`IsActionRequired()` is stuck true, and the outer loop just calls
`ApplyWaitAllUntil` again with the same target time forever. Nothing in the
loop has a way to say "give up, this plan can't make progress."

**Best concrete suspect for *why* the shunting unit goes idle and stays
that way:** in the `SimpleService` case, the first arriving train (`2422`,
`SLT-4`) has a non-predefined (`"other": "Reinigingsperron"`) cleaning task
in its task list. This wasn't checked, but is the most likely candidate —
worth confirming whether `ActionManager`/its generators actually support
`"other"` task types end-to-end, or whether such a task can never produce a
valid generated action, leaving that shunting unit permanently unable to
proceed while the plan waits on it.

## Suggested next steps

1. **Confirm or rule out the "other" task hypothesis.** Instrument
   `GetValidActions`/`ActionManager::Generate` (or just add temporary
   `cout`s) for the `SimpleService` run and see what, if anything, gets
   generated for the shunting unit carrying the `Reinigingsperron` task.
2. **Diff against a plan that completes.** `KleineBinckhorst_30t_random_98s_test`
   finishes fine — compare its task-type mix and action ordering against
   the three that hang to narrow down the structural trigger.
3. **DONE — safety valve added.** Both `LocationEngine::EvaluatePlan`
   overloads (`cTORS/src/engine/Engine.cpp`) now track consecutive
   iterations with no change in `state->GetTime()` and no advance of `it`;
   after `MAX_STALLED_EVALUATE_PLAN_ITERATIONS` (100) such iterations they
   throw `ScenarioFailedException`, which the existing `catch` block turns
   into a normal "Scenario failed." / plan-invalid result instead of an
   infinite loop. Verified against the `data/Bugs/hanging` fixture (also
   wired up as the first `.vscode/launch.json` configuration): the
   previously-infinite run now terminates in well under a second.
4. Once fixed, re-run the four hanging plans above end-to-end and confirm
   they now produce a real result (valid or not) instead of spinning.

## Related

- Compat-patch commits that made this reachable: `55e206b` (tolerant JSON
  parsing), `77c868a`/`a9578dd` (`memberIDs`), `fe7611e` (`"****"`/`null`
  sentinel), `e10061f` (fail-fast on empty `memberIDs`).
- Fixture data: `../scenario-planning-inputs/Location_SimpleService/`,
  `../scenario-planning-inputs/Location_KleineBinckhorst/`.
