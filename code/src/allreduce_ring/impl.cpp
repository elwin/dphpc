#include "allreduce_ring/impl.hpp"

namespace impls::allreduce {

void allreduce_ring::compute(const std::vector<vector>& a_in, const std::vector<vector>& b_in, matrix& result) {
  const auto& a = a_in[rank];
  const auto& b = b_in[rank];

  auto current = matrix::outer(a, b).get_ptr();

  unsigned long chunk_size = (a.size() * b.size()) / num_procs;
  unsigned long last_chunk_size = chunk_size + ((a.size() * b.size()) % num_procs);
  auto destination = (rank + 1) % num_procs;
  auto source = (rank - 1) % num_procs;

  // Send partial results through the ring until everyone has everything
  for (int i = 0; i < num_procs; ++i) {
    auto chunk_index = (rank + i) % num_procs;

    // Determine current chunk offset and length
    auto chunk_offset = chunk_index * chunk_size;
    int chunk_length = 0;
    if (chunk_index == num_procs - 1) {
      chunk_length = (int)last_chunk_size;
    } else {
      chunk_length = (int)chunk_size;
    }

    // Send current chunk to next node
    MPI_Send(current + chunk_offset, chunk_length, MPI_DOUBLE, destination, MPI_ANY_TAG, comm);

    // Calculate chunk length of receiving chunk
    chunk_index = chunk_index - 1 % num_procs;
    chunk_offset = chunk_index * chunk_size;
    if (chunk_index == num_procs - 1) {
      chunk_length = (int)last_chunk_size;
    } else {
      chunk_length = (int)chunk_size;
    }

    // Receive chunk from previous node
    auto recv_chunk = new double[chunk_length];
    MPI_Recv(recv_chunk, chunk_length, MPI_DOUBLE, source, MPI_ANY_TAG, comm, MPI_STATUS_IGNORE);

    // Add received chunk to current matrix
    for (int j = 0; j < chunk_length; ++j) {
      current[chunk_offset + j] = recv_chunk[j] + current[chunk_offset + j];
    }
  }

  // TODO: Distribute partial results among all nodes
  for (int i = 0; i < num_procs; ++i) {

  }
}

} // namespace impls::allreduce
