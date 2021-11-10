#!/bin/bash

# This script runs verify_completed.sh for check_completed.sh

set -e

cd ~/dphpc
source ./euler/init.sh
python ./benchmarks/process.py