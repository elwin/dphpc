#include "allreduce/impl.hpp"

namespace impls::allreduce {

matrix* allreduce::compute() {
  matrix* current = matrix::outer(this->a, this->b);
  auto result = new matrix(this->a.size(), this->b.size());

  MPI_Allreduce(current->get_ptr(), result->get_ptr(), result->dimension(), MPI_DOUBLE, MPI_SUM, this->comm);

  return result;
}

} // namespace impls::allreduce
