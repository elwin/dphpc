. ./scripts/euler/config.sh

while read -r job_id; do

  filename="${job_id}"

  echo $filename

  tail -n +36 "${bb_output_dir}/${filename}" > "${output_dir}/${filename}"

done <$job_list

echo "Collected all files in ${output_dir}"