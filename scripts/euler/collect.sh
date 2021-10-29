. ./scripts/euler/config.sh

job_path="${bb_output_dir}/jobs"

while read -r job_row; do
  repetition=$(echo "${job_row}" | awk -F '/' '{print $1}')
  job_id=$(echo "${job_row}" | awk -F '/' '{print $2}')

  folder="${bb_output_dir}/${repetition}/"
  filename="${job_id}"

  mkdir -p "${folder}"

  tail -n +36 "${folder}/${filename}" >>"${output_dir}/${repetition}.json"

done <"${job_path}"

echo "Collected all files in ${output_dir}"
