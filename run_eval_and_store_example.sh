#!/bin/bash


./build/TORS --mode "EVAL_AND_STORE" \
    --path_location "./examples_kleine_binckhorst" \
    --path_scenario "./examples_kleine_binckhorst/scenario.json" \
    --path_plan "./examples_kleine_binckhorst/plan.json" \
    --path_eval_result "./examples_kleine_binckhorst/results/evaluation_results.txt" \
    --plan_type "Solver"
