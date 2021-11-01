#!/bin/bash

echo "This script is deprecated. Use verify.py instead"

. ./scripts/euler/config.sh

error=false

while read -r job_row; do

  repetition=$(echo "${job_row}" | awk -F '/' '{print $1}')
  job_id=$(echo "${job_row}" | awk -F '/' '{print $2}')

  status=$(bbjobs "${job_id}" | grep Status)

  if [[ $status == *"PENDING"* ]]; then
    echo "Job ${job_id} is still pending!"
    error=true
    continue
  fi

  if [[ $status == *"RUNNING"* ]]; then
    echo "Job ${job_id} is still running!"
    error=true
    continue
  fi

  if [[ $status != *"DONE"* ]]; then
    echo "Job ${job_id} has an unexpected error code!"
    error=true
    continue
  fi

  echo "Job ${job_id} (repetition ${repetition}) has completed"

done <"$job_file"

if [ "${error}" = true ]; then
  exit 1
fi

echo "All jobs are finished!"
