#!/bin/bash
set -e
set -u

bb_output_dir="../bb_jobs"
output_dir="../parsed"
job_file="${bb_output_dir}/jobs"
job_overview_file="${bb_output_dir}/overview"
trash_dir="../trash"

mkdir -p "${bb_output_dir}" "${output_dir}" "${trash_dir}"

code_dir='./code'
build_dir="./code/build_output"
