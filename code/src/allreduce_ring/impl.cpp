#include "allreduce_ring/impl.hpp"

#include <iostream>

namespace impls::allreduce {
void allreduce_ring::compute(const std::vector<vector>& a_in, const std::vector<vector>& b_in, matrix& result) {
  const auto& a = a_in[rank];
  const auto& b = b_in[rank];

  result.set_outer_product(a, b);
  auto current = result.get_ptr();

  unsigned long chunk_size = (a.size() * b.size()) / num_procs;
  unsigned long last_chunk_size = chunk_size + ((a.size() * b.size()) % num_procs);
  auto destination = (rank + 1) % num_procs;
  auto source = (rank - 1) % num_procs;

  // Send partial results through the ring until everyone has everything
  for (int i = 0; i < num_procs - 1; ++i) {
    // Determine current chunk offset and length
    auto snd_chunk_index = (rank + num_procs - i) % num_procs;
    auto snd_chunk_offset = snd_chunk_index * chunk_size;
    int snd_chunk_length = snd_chunk_index == num_procs - 1 ? (int)last_chunk_size : (int)chunk_size;

    // Calculate chunk length of receiving chunk
    auto rcv_chunk_index = (snd_chunk_index + num_procs - 1) % num_procs;
    auto rcv_chunk_offset = rcv_chunk_index * chunk_size;
    int rcv_chunk_length = rcv_chunk_index == num_procs - 1 ? (int)last_chunk_size : (int)chunk_size;

    // Send current chunk to next node
    MPI_Request sendRequest;
    mpi_timer(MPI_Isend, current + snd_chunk_offset, snd_chunk_length, MPI_DOUBLE, destination, 0, comm, &sendRequest);

    // Receive chunk from previous node
    auto recv_chunk = new double[rcv_chunk_length];
    mpi_timer(MPI_Recv, recv_chunk, rcv_chunk_length, MPI_DOUBLE, source, 0, comm, MPI_STATUS_IGNORE);

    // The message should be received so we can wait on it
    mpi_timer(MPI_Wait, &sendRequest, MPI_STATUS_IGNORE);

    // Add received chunk to current matrix
    for (int j = 0; j < rcv_chunk_length; ++j) {
      current[rcv_chunk_offset + j] += recv_chunk[j];
    }

    delete[] recv_chunk;
  }

  // At this point the current node should have the result of the chunk with index (rank + 1).
  // We then have to distribute all result chunks
  for (int i = 0; i < num_procs - 1; ++i) {
    // Determine current chunk offset and length
    auto snd_chunk_index = (rank + num_procs - i + 1) % num_procs;
    auto snd_chunk_offset = snd_chunk_index * chunk_size;
    int snd_chunk_length = snd_chunk_index == num_procs - 1 ? (int)last_chunk_size : (int)chunk_size;

    // Calculate chunk length of receiving chunk
    auto rcv_chunk_index = (snd_chunk_index + num_procs - 1) % num_procs;
    auto rcv_chunk_offset = rcv_chunk_index * chunk_size;
    int rcv_chunk_length = rcv_chunk_index == num_procs - 1 ? (int)last_chunk_size : (int)chunk_size;

    // Send current chunk to next node
    MPI_Request sendRequest;
    mpi_timer(MPI_Isend, current + snd_chunk_offset, snd_chunk_length, MPI_DOUBLE, destination, 0, comm, &sendRequest);

    // Receive chunk from previous node
    mpi_timer(MPI_Recv, current + rcv_chunk_offset, rcv_chunk_length, MPI_DOUBLE, source, 0, comm, MPI_STATUS_IGNORE);

    // The message should be received so we can wait on it
    mpi_timer(MPI_Wait, &sendRequest, MPI_STATUS_IGNORE);
  }
}

} // namespace impls::allreduce
