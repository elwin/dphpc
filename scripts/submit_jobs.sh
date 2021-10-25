#!/bin/bash

set -e # Exit on first failure

# Bash script for running jobs on the euler cluster.
# Make sure you prepared ssh according to the preparations in the readme file.

ssh -t euler < run_benchmark.sh