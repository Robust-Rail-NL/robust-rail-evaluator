#!/bin/bash


./build/TORS --mode "EVAL" \
    --path_location "./data/Demo/TUSS-Instance-Generator/kleine_binckhorst" \
    --path_scenario "./data/Demo/TUSS-Instance-Generator/kleine_binckhorst/scenario.json" \
    --path_plan "./data/Demo/TUSS-Instance-Generator/kleine_binckhorst/plan.json" \
    --plan_type "Solver"
