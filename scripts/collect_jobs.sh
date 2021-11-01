#!/bin/bash

set -e # Exit on first failure

# Execute on LOCAL machine.

# Script checks whether the experiments is finished.
# If not finished:  exit
# If finished:      collect files and copy to local machine
ssh -t euler < ./scripts/euler_wrapper/run_verify_completed.sh
scp -r euler:~/dphpc/results/tmp results