#!/bin/bash


./build/TORS --mode "EVAL" \
    --path_location "./examples_kleine_binckhorst" \
    --path_scenario "./examples_kleine_binckhorst/scenario.json" \
    --path_plan "./examples_kleine_binckhorst/plan.json" \
    --plan_type "Solver"
