#include "allreduce/impl.hpp"

#include <iostream>

#include "dsop.h"
namespace impls::allreduce {

matrix* allreduce::compute() {
  matrix* current = matrix::outer(this->a, this->b);
  auto result = new matrix(this->a.size(), this->b.size());

  MPI_Allreduce(current->get_ptr(), result->get_ptr(), result->dimension(), MPI_DOUBLE, MPI_SUM, this->comm);

  return result;
}

std::unique_ptr<double[]> run(
    MPI_Comm comm, std::unique_ptr<double[]> A, int N, std::unique_ptr<double[]> B, int M, int, int) {
  auto G = std::make_unique<double[]>(N * M);

  for (int i = 0; i < N; i++) {
    for (int j = 0; j < M; j++) {
      G[i * M + j] = A[i] * B[j];
    }
  }

  auto result = std::make_unique<double[]>(N * M);
  MPI_Allreduce(G.get(), result.get(), N * M, MPI_DOUBLE, MPI_SUM, comm);

  return G;
}

} // namespace impls::allreduce
