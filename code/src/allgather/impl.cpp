#include "allgather/impl.hpp"

namespace impls::allgather {

matrix allgather::compute(const std::vector<vector>& a_in, const std::vector<vector>& b_in) {
  const auto& a = a_in[rank];
  const auto& b = b_in[rank];

  auto rec_a = vector(N * num_procs);
  MPI_Allgather(a.data(), N, MPI_DOUBLE, rec_a.data(), N, MPI_DOUBLE, comm);

  auto rec_b = vector(M * num_procs);
  MPI_Allgather(b.data(), M, MPI_DOUBLE, rec_b.data(), M, MPI_DOUBLE, comm);

  auto result = matrix(N, M);
  for (int i = 0; i < num_procs; i++) {
    for (int x = 0; x < N; x++) {
      for (int y = 0; y < M; y++) {
        result.get(x, y) += rec_a[i * N + x] * rec_b[i * M + y];
      }
    }
  }
  return result;
}

} // namespace impls::allgather
