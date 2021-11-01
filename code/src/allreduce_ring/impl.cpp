#include "allreduce_ring/impl.hpp"

namespace impls::allreduce {

void allreduce_ring::compute(const std::vector<vector>& a_in, const std::vector<vector>& b_in, matrix& result) {
  const auto& a = a_in[rank];
  const auto& b = b_in[rank];

  auto current = matrix::outer(a, b);
  mpi_timer(MPI_Allreduce, current.get_ptr(), result.get_ptr(), result.dimension(), MPI_DOUBLE, MPI_SUM, comm);
}

} // namespace impls::allreduce
