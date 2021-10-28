#include "allreduce/impl.hpp"

namespace impls::allreduce {

matrix allreduce::compute(const std::vector<vector>& a_in, const std::vector<vector>& b_in) {
  const auto& a = a_in[rank];
  const auto& b = b_in[rank];

  auto current = matrix::outer(a, b);
  auto result = matrix(N, M);

  MPI_Allreduce(current.get_ptr(), result.get_ptr(), result.dimension(), MPI_DOUBLE, MPI_SUM, comm);

  return result;
}

} // namespace impls::allreduce
