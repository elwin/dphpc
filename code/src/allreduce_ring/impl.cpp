#include "allreduce_ring/impl.hpp"

#include <iostream>

namespace impls::allreduce {
void allreduce_ring::compute(const std::vector<vector>& a_in, const std::vector<vector>& b_in, matrix& result) {
  const auto& a = a_in[rank];
  const auto& b = b_in[rank];

  auto current_matrix = matrix::outer(a, b);
  auto current = current_matrix.get_ptr();

  unsigned long chunk_size = (a.size() * b.size()) / num_procs;
  unsigned long last_chunk_size = chunk_size + ((a.size() * b.size()) % num_procs);
  auto destination = (rank + 1) % num_procs;
  auto source = (rank - 1) % num_procs;

  std::cout << "[" << rank << "] OP: " << current[0] << std::endl;

  // Send partial results through the ring until everyone has everything
  for (int i = 0; i < num_procs - 1; ++i) {
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
    mpi_timer(MPI_Send, current + chunk_offset, chunk_length, MPI_DOUBLE, destination, 0, comm);

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

    // Add received chunk to current matrix
    for (int j = 0; j < chunk_length; ++j) {
      current[chunk_offset + j] = current[chunk_offset + j] + recv_chunk[j];
    }

    std::cout << "[" << rank << "] "
              << "Iteration " << i << " (chunk_offset=" << chunk_offset << "): " << current[chunk_offset] << std::endl;
  }

  std::cout << "[" << rank << "] " << current[chunk_size * ((rank + 1) % num_procs)] << std::endl;

  // At this point the current node should have the result of the chunk with index (rank + 1).
  // We then have to distribute all result chunks
  for (int i = 0; i < num_procs - 1; ++i) {
    auto chunk_index = (rank - i + 1) % num_procs;

    auto chunk_offset = chunk_index * chunk_size;
    int chunk_length = 0;
    if (chunk_index == num_procs - 1) {
      chunk_length = (int)last_chunk_size;
    } else {
      chunk_length = (int)chunk_size;
    }

    // Send current chunk to next node
    mpi_timer(MPI_Send, current + chunk_offset, chunk_length, MPI_DOUBLE, destination, 0, comm);

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

    // Replace received chunk in result
    for (int j = 0; j < chunk_length; ++j) {
      current[chunk_offset + j] = recv_chunk[j];
    }
  }

  // Copy result into final matrix
  auto result_ptr = result.get_ptr();
  for (int i = 0; i < N * M; ++i) {
    result_ptr[i] = current[i];
  }
}

} // namespace impls::allreduce
