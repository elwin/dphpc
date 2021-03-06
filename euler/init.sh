#!/bin/bash

# call this file using `source init.sh`

source /cluster/apps/local/env2lmod.sh
module load gcc/9.3.0 cmake/3.20.3 openmpi/4.0.2 python/3.6.5
pip list | grep dataclasses || pip install --user dataclasses
pip list | grep matplotlib || pip install --user matplotlib
pip list | grep seaborn || pip install --user seaborn
