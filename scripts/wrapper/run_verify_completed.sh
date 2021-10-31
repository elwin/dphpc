#!/bin/bash

# This script runs verify_completed.sh for check_completed.sh

set -e

cd ~/dphpc
source ./scripts/euler/init.sh
python ./scripts/euler/verify.py
python ./scripts/euler/collect.py