. ./scripts/euler/config.sh

while read -r job_row; do
  job_id=$(echo "${job_row}" | awk -F '/' '{print $2}')

  bkill "${job_id}"

done <"${job_file}"
