#!/bin/bash

set -e # Exit on first failure

# Executed on LOCAL machine.
# To run jobs on euler cluster, issued on local machine.

# Make sure you prepared ssh according to the preparations in the readme file.

ssh -t euler < ./scripts/euler_wrapper/run_benchmark.sh