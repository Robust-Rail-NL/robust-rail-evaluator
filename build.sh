#!/bin/bash

# This file is intended to be run from inside a Docker container build from
# the included Dockerfile.

conda activate my-proto-env
mkdir /workspace/robust-rail-generator/build
cd /workspace/robust-rail-generator/build
cmake .. -DCONDA_ENV="/opt/conda/envs/my_proto_env"
cmake --build .
