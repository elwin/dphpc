NODES=${NODES:-8} # Can be overriden by specifying an env variable, e.g. NODES=16
REPETITIONS=25
ALGORITHMS=(allgather allreduce allreduce-ring g-rabenseifner-allgather g-rabenseifner-subgroup-2)
VECTOR_SIZES=(1000 2000 3000 4000 5000 6000 7000 8000)
BINARY="./code/build_output/main"
OUTPUT="./results"
mkdir ${OUTPUT}

for VECTOR_SIZE in "${VECTOR_SIZES[@]}"; do
  for ALGORITHM in "${ALGORITHMS[@]}"; do
      mpirun -np "${NODES}" "${BINARY}" -n "${VECTOR_SIZE}" -m "${VECTOR_SIZE}" -t "${REPETITIONS}" -i "${ALGORITHM}" >> "${OUTPUT}/results-${NODES}.json" 2>> "${OUTPUT}/err-${NODES}.log"
    done
done
