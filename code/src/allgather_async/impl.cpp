#include "allgather_async/impl.hpp"

namespace impls::allgather_async {

void allgather_async::compute(const std::vector<vector>& a_in, const std::vector<vector>& b_in, matrix& result) {
  const auto& a = a_in[rank];
  const auto& b = b_in[rank];

  auto rec_a = vector(N * num_procs);
  mpi_timer(MPI_Allgather, a.data(), N, MPI_DOUBLE, rec_a.data(), N, MPI_DOUBLE, comm);

  auto rec_b = vector(M * num_procs);
  mpi_timer(MPI_Allgather, b.data(), M, MPI_DOUBLE, rec_b.data(), M, MPI_DOUBLE, comm);

  for (int i = 0; i < num_procs; i++) {
    for (int x = 0; x < N; x++) {
      double tmp = rec_a[i * N + x];
      for (int y = 0; y < M; y++) {
        result.get(x, y) += tmp * rec_b[i * M + y];
      }
    }
  }
}

} // namespace impls::allgather_async
