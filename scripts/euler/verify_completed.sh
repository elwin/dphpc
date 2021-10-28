source config.sh

while read -r job_id; do
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

done <$job_list

echo "All jobs are finished!"
