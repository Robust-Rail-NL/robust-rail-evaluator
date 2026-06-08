#!/bin/bash

# This file is intended to be run from inside a Docker container build from
# the included Dockerfile.

eval "$(conda shell.bash hook)"

conda init
conda activate my-proto-env
mkdir /workspace/robust-rail-evaluator/build
cd /workspace/robust-rail-evaluator/build
cmake .. -DCONDA_ENV="/opt/conda/envs/my_proto_env"
cmake --build .
