#include "allreduce_ring/impl.hpp"

#include <iostream>

namespace impls::allreduce {
void allreduce_ring::compute(const std::vector<vector>& a_in, const std::vector<vector>& b_in, matrix& result) {
  const auto& a = a_in[rank];
  const auto& b = b_in[rank];

  set_outer_product(result, a, b);
  auto current = result.get_ptr();

  unsigned long chunk_size = (a.size() * b.size()) / num_procs;
  unsigned long last_chunk_size = chunk_size + ((a.size() * b.size()) % num_procs);
  auto destination = (rank + 1) % num_procs;
  auto source = (rank - 1) % num_procs;

  // Send partial results through the ring until everyone has everything
  for (int i = 0; i < num_procs - 1; ++i) {
    auto chunk_index = (rank + num_procs - i) % num_procs;

    // Determine current chunk offset and length
    auto chunk_offset = chunk_index * chunk_size;
    int chunk_length = 0;
    if (chunk_index == num_procs - 1) {
      chunk_length = (int)last_chunk_size;
    } else {
      chunk_length = (int)chunk_size;
    }

    // Send current chunk to next node
    MPI_Request sendRequest;
    mpi_timer(MPI_Isend, current + chunk_offset, chunk_length, MPI_DOUBLE, destination, 0, comm, &sendRequest);

    // Calculate chunk length of receiving chunk
    chunk_index = (chunk_index + num_procs - 1) % num_procs;
    chunk_offset = chunk_index * chunk_size;
    if (chunk_index == num_procs - 1) {
      chunk_length = (int)last_chunk_size;
    } else {
      chunk_length = (int)chunk_size;
    }

    // Receive chunk from previous node
    auto recv_chunk = new double[chunk_length];
    mpi_timer(MPI_Recv, recv_chunk, chunk_length, MPI_DOUBLE, source, 0, comm, MPI_STATUS_IGNORE);

    // The message should be received so we can wait on it
    mpi_timer(MPI_Wait, &sendRequest, MPI_STATUS_IGNORE);

    // Add received chunk to current matrix
    for (int j = 0; j < chunk_length; ++j) {
      current[chunk_offset + j] += recv_chunk[j];
    }

    delete[] recv_chunk;
  }

  // At this point the current node should have the result of the chunk with index (rank + 1).
  // We then have to distribute all result chunks
  for (int i = 0; i < num_procs - 1; ++i) {
    auto chunk_index = (rank + num_procs - i + 1) % num_procs;

    auto chunk_offset = chunk_index * chunk_size;
    int chunk_length = 0;
    if (chunk_index == num_procs - 1) {
      chunk_length = (int)last_chunk_size;
    } else {
      chunk_length = (int)chunk_size;
    }

    // Send current chunk to next node
    MPI_Request sendRequest;
    mpi_timer(MPI_Isend, current + chunk_offset, chunk_length, MPI_DOUBLE, destination, 0, comm, &sendRequest);

    // Calculate chunk length of receiving chunk
    chunk_index = (chunk_index + num_procs - 1) % num_procs;
    chunk_offset = chunk_index * chunk_size;
    if (chunk_index == num_procs - 1) {
      chunk_length = (int)last_chunk_size;
    } else {
      chunk_length = (int)chunk_size;
    }

    // Receive chunk from previous node
    mpi_timer(MPI_Recv, current + chunk_offset, chunk_length, MPI_DOUBLE, source, 0, comm, MPI_STATUS_IGNORE);

    // The message should be received so we can wait on it
    mpi_timer(MPI_Wait, &sendRequest, MPI_STATUS_IGNORE);
  }
}

} // namespace impls::allreduce
