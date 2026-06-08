#!/bin/bash

cd robust-rail-evaluator
conda env create -f env.yml
conda init
source ~/.bashrc
conda activate my-proto-env
mkdir build
cd build
cmake .. -DCONDA_ENV="/opt/conda/envs/my_proto_env"
cmake --build .
