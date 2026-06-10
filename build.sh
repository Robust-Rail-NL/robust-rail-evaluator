#!/bin/bash

# This file is intended to be run from inside a Docker container built from
# the included Dockerfile.

eval "$(conda shell.bash hook)"

conda activate my_proto_env
mkdir /workspace/robust-rail-evaluator/build
cd /workspace/robust-rail-evaluator/build
cmake .. -DCONDA_ENV="/opt/conda/envs/my_proto_env"
cmake --build .
