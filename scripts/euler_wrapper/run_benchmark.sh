#!/bin/bash

# This script runs benchmark.sh for submit_jobs.sh

set -e

cd ~/dphpc
source ./scripts/euler/init.sh
python ./scripts/euler/benchmark.py