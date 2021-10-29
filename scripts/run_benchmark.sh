#!/bin/bash

set -e

cd ~/dphpc
source /cluster/apps/local/env2lmod.sh
module load gcc/8.2.0 cmake/3.20.3 openmpi/4.0.2
./scripts/euler/benchmark.sh -e cluster