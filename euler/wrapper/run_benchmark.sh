#!/bin/bash

# This script runs benchmark.sh for submit_jobs.sh

set -e

cd ~/dphpc
source ./euler/init.sh
(cd code; make clean build)
python ./benchmarks/benchmark.py --mode euler --clean

# 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
# git pull https://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@github.com/elwin/dphpc
# git fetch https://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@github.com/elwin/dphpc
# git checkout master https://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@github.com/elwin/dphpc