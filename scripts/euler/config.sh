# not meant to be run directly

set -e
set -u

results_dir="results/tmp"
bb_output_dir="${results_dir}/bb_jobs"
output_dir="${results_dir}/parsed"
job_file="${bb_output_dir}/jobs"
job_overview_file="${bb_output_dir}/overview.jsonl"
code_dir='./code'
build_dir="./code/build_output"

mkdir -p "${bb_output_dir}" "${output_dir}"
