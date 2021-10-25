#include "allgather/impl.hpp"

namespace impls::allgather {

matrix allgather::compute() {
  // Create new combined vector to send
  auto send_vector_length = a.size() + b.size();
  vector send_vector;
  send_vector.reserve(send_vector_length);
  send_vector.insert(send_vector.end(), a.begin(), a.end());
  send_vector.insert(send_vector.end(), b.begin(), b.end());

  // Create receive vector
  auto rec_vector = vector(send_vector_length * num_procs);

  // Distribute all vectors to all nodes
  MPI_Allgather(
      send_vector.data(), send_vector_length, MPI_DOUBLE, rec_vector.data(), send_vector_length, MPI_DOUBLE, comm);

  // Calculate sum of products on all nodes
  auto result = matrix(a.size(), b.size());
  for (int i = 0; i < num_procs; i++) {
    for (size_t x = 0; x < a.size(); x++) {
      for (size_t y = 0; y < b.size(); y++) {
        result.get(x, y) += rec_vector[i * send_vector_length + x] * rec_vector[i * send_vector_length + a.size() + y];
      }
    }
  }
  return result;
}

} // namespace impls::allgather
