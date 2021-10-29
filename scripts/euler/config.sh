#!/bin/bash
set -e
set -u

echo "config"

bb_output_dir="../bb_jobs"
output_dir="../parsed"

mkdir -p "${bb_output_dir}" "${output_dir}"

code_dir='./code'
build_dir="./code/build_output"
