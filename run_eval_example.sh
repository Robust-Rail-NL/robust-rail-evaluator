#!/bin/bash


./build/TORS --mode "EVAL" \
    --path_location "./example_kleine_binckhorst" \
    --path_scenario "./example_kleine_binckhorst/scenario.json" \
    --path_plan "./example_kleine_binckhorst/plan.json" \
    --plan_type "Solver"
