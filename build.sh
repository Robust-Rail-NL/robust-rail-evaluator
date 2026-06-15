#!/bin/bash

# This file is intended to be run from inside a Docker container built from
# the included Dockerfile.

mkdir -p /workspace/build
cd /workspace/build
cmake ..
cmake --build .
