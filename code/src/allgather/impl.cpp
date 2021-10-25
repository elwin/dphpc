#include "allgather/impl.hpp"

namespace impls::allgather {

matrix allgather::compute() {
  // Create new combined vector to send
  auto send_vector_length = a.size() + b.size();
  auto send_vector = vector(send_vector_length);
  for (unsigned long i = 0; i < a.size(); ++i) {
    send_vector[i] = a[i];
  }
  for (unsigned long i = 0; i < b.size(); ++i) {
    send_vector[i + a.size()] = b[i];
  }

  // Create receive vector
  auto rec_vector_length = (a.size() + b.size()) * num_procs;
  auto rec_vector = vector(rec_vector_length);

  // Distribute all vectors to all nodes
  MPI_Allgather(&send_vector[0], send_vector_length, MPI_DOUBLE, &rec_vector[0], send_vector_length, MPI_DOUBLE, comm);

  // Calculate sum of products on all nodes
  auto result = matrix(a.size(), b.size());
  for (int i = 0; i < num_procs; i++) {
    // Split vectors again
    auto A = vector(a.size());
    auto B = vector(b.size());

    for (unsigned int j = 0; j < send_vector_length; j++) {
      if (j < a.size()) {
        A[j] = rec_vector[i * send_vector_length + j];
      } else {
        B[j - a.size()] = rec_vector[i * send_vector_length + j];
      }
    }

    auto current = matrix::outer(A, B);

    result.add(current);
  }

  return result;
}

} // namespace impls::allgather
