#!/bin/bash

echo "This script is deprecated. Use collect.py instead"

. ./scripts/euler/config.sh

mkdir -p "${output_dir}"

while IFS= read -r job_row; do
  repetition=$(echo "${job_row}" | awk -F '/' '{print $1}')
  job_id=$(echo "${job_row}" | awk -F '/' '{print $2}')

  source_file="${bb_output_dir}/${repetition}/${job_id}"
  dest_file="${output_dir}/${repetition}.json"

  # echo "jobID=$job_id, repetition=$repetition, row=$job_row, source-file=$source_file, dest_file=$dest_file"

  # use || true to prevent exit from grep when no result was found
  out=$(cat $source_file | { grep -i '{.*}' || true; })
  if [ ! -z "$out" ]
  then
    echo $out >> $dest_file
  else
    grep_string=".*${job_id}.*"
    echo "file=$source_file, configuration=$(cat $job_overview_file | { grep -i $grep_string || true; }), error-message=$(cat $source_file | { grep -i '.*job killed.*' || true; })"
  fi

  # Buggy?
  #  tail -n +36 $source_file | head -n -6 >> $dest_file

done < "${job_file}"

echo "Collected all files in ${output_dir}"
