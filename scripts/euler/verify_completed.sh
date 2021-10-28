. config.sh

job_path="${bb_output_dir}/jobs"

while read -r job_row; do
  repetition=$(echo "${job_row}" | awk -F '/' '{print $1}')
  job_id=$(echo "${job_row}" | awk -F '/' '{print $2}')

  status=$(bbjobs "${job_id}" | grep Status)

  if [[ $status == *"PENDING"* ]]; then
    echo "Job ${job_id} is still pending!"
    exit 1
  fi

  if [[ $status == *"RUNNING"* ]]; then
    echo "Job ${job_id} is still running!"
    exit 1
  fi

  if [[ $status != *"DONE"* ]]; then
    echo "Job ${job_id} has an unexpected error code!"
    exit 1
  fi

  echo "Job ${job_id} (repetition ${repetition}) has completed"

done <"$job_path"

echo "All jobs are finished!"
