#!/bin/bash

# This script runs benchmark.sh for submit_jobs.sh

set -e

cd ~/dphpc
source ./euler/init.sh
(cd code; make clean build)
python ./benchmarks/benchmark.py --mode euler --grouped --clean