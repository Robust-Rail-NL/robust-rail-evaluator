#!/bin/bash


./build/TORS --mode "EVAL_AND_STORE" \
    --path_location "./example_kleine_binckhorst" \
    --path_scenario "./example_kleine_binckhorst/scenario.json" \
    --path_plan "./example_kleine_binckhorst/plan.json" \
    --path_eval_result "./example_kleine_binckhorst/results/evaluation_results.txt" \
    --plan_type "Solver"
