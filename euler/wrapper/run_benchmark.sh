#!/bin/bash

# This script runs benchmark.sh for submit_jobs.sh

set -e

cd ~/dphpc
source ./euler/init.sh
python ./benchmarks/benchmark.py