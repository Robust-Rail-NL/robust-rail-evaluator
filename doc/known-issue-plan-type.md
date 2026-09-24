# Known issue: the wrong `--plan_type` silently evaluates nothing and reports "valid"

**Status:** partially fixed. `--plan_type` now validates its value and
defaults to `Solver` when omitted (see "Suggested next steps" below). Whether
to retire or rename `Evaluator` itself is still open.

## Symptom

`TORS --mode EVAL --plan_type Evaluator ...` against a plan file shaped like
robust-rail-solver's output (`{"schemaVersion":.., "actions":[...]}` — the
shape every real example plan in this repo and every external bug report's
attached plan file uses) does not error. It prints `RunResult Protobuf is
empty or not initialized.` and then unconditionally `The plan is valid` —
regardless of what the plan actually contains, including a genuinely invalid
plan, an empty plan, or complete garbage.

Confirmed directly: feeding literal `{"actions":[]}` and `{"nonsense":true}`
through `--plan_type Evaluator` both report "The plan is valid".

A related, unvalidated case: `--plan_type` accepts *any* string with no
validation at all. `main.cpp`'s `if (plan_type == "Solver") {...} else if
(plan_type == "Evaluator") {...}` has no final `else` — a typo'd or unknown
value (`--plan_type Solverr`) falls through silently, skips both branches
entirely, and prints nothing about plan evaluation at all, valid or
otherwise.

## Root cause

`--plan_type Evaluator` and `--plan_type Solver` parse two genuinely
different top-level JSON shapes:

- **`Solver`** (`ParseHIP_PlanFromJson` → `RunResult::CreateRunResult(pb_hip_plan, ...)`,
  `cTORS/src/main.cpp` ~line 96): the HIP-style shape — `{"schemaVersion":..,
  "actions":[{"shuntingUnit":{...}, "taskType":{"predefined":...},
  "location":.., "resources":[...]}, ...]}`. This is what robust-rail-solver
  actually emits, and it's what every example plan file and every external
  bug reporter's attached plan is shaped like. `robust-rail-general`'s
  `run_evaluator.py` hardcodes this value — the automated pipeline never
  uses `Evaluator` at all.
- **`Evaluator`** (`GetRunResultProto` → `RunResult::CreateRunResult(&location,
  pb_run_external)`, ~line 131): a `PBRun` object — TORS's own internal
  round-trip format (see `Scenario.h`'s comment: "Used only for TORS's
  internal Run round-trip format"), with top-level `location`/`scenario`/
  `plan`/`feasible` fields, produced by `RunResult::Serialize` at the end of
  an `INTER` (interactive) session. It is not the format any real origin
  (solver, planner, or a hand-written bug report) emits.

Feeding a `Solver`-shaped file to `--plan_type Evaluator` doesn't throw
because JSON parsing here uses `ignore_unknown_fields = true` (see
`Utils.h`/`parse_json_to_pb`, added for interop with a Pydantic-based JSON
generator in a sibling repo). None of `schemaVersion`/`actions` match
`PBRun`'s fields, so it silently parses into an empty `PBRun` with zero
actions. `LocationEngine::EvaluatePlan`'s action loop then has nothing to
iterate and trivially returns `true`.

This is the same failure family as `doc/known-issue-plan-evaluation-hang.md`'s
"Why this isn't a regression" section, where an old TORS build fed a
new-format plan similarly parsed to an empty scenario and reported "valid".
Permissive/`ignore_unknown_fields` JSON parsing plus no shape validation at
the entry point means malformed-for-this-endpoint input reads as "nothing to
check" rather than "wrong format" — twice now, in two different places.

## Why this matters

Given the tool is literally *named* "the evaluator", `--plan_type Evaluator`
reads as the obviously-correct choice to anyone testing a plan by hand
without already knowing this distinction — and picking it produces a clean,
confident "valid" result instead of an error. This is exactly what happened
while investigating GitHub issue #13: every plan-level result produced via
`--plan_type Evaluator` against the reporter's attached (`Solver`-shaped)
plan file was meaningless, and only scenario-level checks (which run before
plan parsing, independent of plan shape) were real. Switching to the
correct `--plan_type Solver` immediately reproduced the actual bug,
byte-for-byte matching the reporter's error text.

## Suggested next steps

1. **DONE — validate the flag's value.** Both the `EVAL` and `EVAL_AND_STORE`
   branches of `main.cpp` now have a final `else` that rejects an
   unrecognized `--plan_type` with a clear error and a non-zero exit code,
   instead of silently printing nothing.
2. **DONE — default `--plan_type` to `Solver`.** It was a required argument
   with no default; omitting it now defaults to `Solver`. This only helps
   the omission case, not passing `Evaluator` explicitly.
3. **Still open — retire or rename `Evaluator`.** It has no real origin besides TORS's
   own `INTER`-mode round-trip output, and its name collides confusingly
   with the tool's own name. Options to weigh: drop it outright (breaking,
   if anyone relies on the `INTER` round-trip workflow); rename it to
   something that doesn't read as "the normal/default choice" (e.g. `Run` or
   `InternalRun`), keeping `Evaluator` as a deprecated alias that still works
   but prints a warning for one release; or keep the name but print a loud
   warning whenever it's selected, pointing at this doc.
4. Update `README.md` (`--plan_type "Evaluator"/"Solver"` section, ~line
   105/124) and `main.cpp`'s usage comment to match whatever's decided.

## Related

- [[known-issue-plan-evaluation-hang.md]] — the same silent-empty-plan
  failure family, previously found on the old-TORS-vs-new-format-JSON axis
  rather than the `--plan_type` axis.
- GitHub issues #12 and #13 (external bug reports) — both attached
  `Solver`-shaped plan files; only issue #13's investigation surfaced this,
  because issue #12's failure is a scenario-level check that runs before
  plan parsing and so is unaffected by plan shape.
