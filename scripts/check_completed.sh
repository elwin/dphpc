#!/bin/bash

set -e # Exit on first failure

# Executed on local machine.
# To check if jobs on euler are finished.

# Make sure you prepared ssh according to the preparations in the readme file.

ssh -t euler < ./scripts/wrapper/run_verify_completed.sh