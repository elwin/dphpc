NODES=8
REPETITIONS=25
ALGORITHMS=(allgather allreduce allreduce-ring g-rabenseifner-allgather g-rabenseifner-subgroup-2 g-rabenseifner-subgroup-4 g-rabenseifner-subgroup-8)
VECTOR_SIZES=(1000 2000 3000 4000 5000 6000 7000 8000)
BINARY="./code/build_output/main"

for VECTOR_SIZE in "${VECTOR_SIZES[@]}"; do
  for ALGORITHM in "${ALGORITHMS[@]}"; do
      mpirun -np "${NODES}" "${BINARY}" -n "${VECTOR_SIZE}" -m "${VECTOR_SIZE}" -t "${REPETITIONS}" -i "${ALGORITHM}" >> "out-${NODES}.json" 2>> "err-${NODES}.log"
    done
done