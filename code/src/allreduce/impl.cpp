#include "allreduce/impl.hpp"

namespace impls::allreduce {

matrix allreduce::compute() {
  auto current = matrix::outer(a, b);
  auto result = matrix(a.size(), b.size());

  MPI_Allreduce(current.get_ptr(), result.get_ptr(), result.dimension(), MPI_DOUBLE, MPI_SUM, comm);

  return result;
}

} // namespace impls::allreduce
