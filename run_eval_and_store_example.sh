#!/bin/bash


./build/TORS --mode "EVAL_AND_STORE" \
    --path_location "./data/Demo/TUSS-Instance-Generator/kleine_binckhorst" \
    --path_scenario "./data/Demo/TUSS-Instance-Generator/kleine_binckhorst/scenario.json" \
    --path_plan "./data/Demo/TUSS-Instance-Generator/kleine_binckhorst/plan.json" \
    --path_eval_result "./data/Demo/TUSS-Instance-Generator/kleine_binckhorst/results/evaluation_results.txt" \
    --plan_type "Solver"
