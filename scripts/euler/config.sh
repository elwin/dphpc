#!/bin/bash
set -e
set -u

echo "config"

bb_output_dir="../bb_jobs"
output_dir="../parsed"
job_list="../jobs"

mkdir -p "${bb_output_dir}" "${output_dir}"

run_benchmark_dir='~/dphpc'
code_dir='./code'
build_dir="./code/build_output"
