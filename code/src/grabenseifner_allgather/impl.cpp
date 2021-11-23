#include "grabenseifner_allgather/impl.hpp"

#include <math.h>

#include <cmath>

namespace impls::grabenseifner_allgather {

/**
 * Generalized Rabenseifner using two allgather rounds
 */
void grabenseifner_allgather::compute(
    const std::vector<vector>& a_in, const std::vector<vector>& b_in, matrix& result) {
  const auto& a = a_in[rank];
  const auto& b = b_in[rank];

  // Check if number of processes assumption true
  if (num_procs < 1) {
    fprintf(stderr, "%d: Starting Generalized Rabenseifner numprocs=%d\n", rank, num_procs);
    return;
  }
  size_t n_processors = num_procs;
  size_t n_rows = (size_t)a.size();
  size_t n_cols = (size_t)b.size();
  size_t my_rank = (size_t)rank;
  // partition the rows of the final matrix among all processes
  // If there are not enough rows --> divide rows among first n_rows processes
  size_t my_n_rows = n_rows / n_processors;
  size_t last_n_rows = n_rows - ((n_processors - 1) * my_n_rows);
  size_t residual_rows = last_n_rows - my_n_rows;
  size_t last_chunk_start_row = n_processors * my_n_rows;
  size_t my_start_row = my_n_rows * rank;
  size_t my_block_size = my_n_rows * n_cols;
  //  int i_compute = 1; // TODO
  if (n_rows < n_processors) {
    my_n_rows = 1;
    my_start_row = rank;
    //    if (my_rank >= n_rows) {
    //      i_compute = 0; // TODO
    //    }
  }
  bool special_last_block = (my_n_rows != last_n_rows);
  if (my_rank == n_processors - 1) {
    my_n_rows = last_n_rows;
  }
  // send and receive buffers
  // to avoid two broadcast rounds --> append vectors a, b
  size_t appended_vec_size = n_rows + n_cols;
  vector appended_vectors(appended_vec_size);
  double* appended_vecs = appended_vectors.data();
  const double* a_ptr = a.data();
  const double* b_ptr = b.data();
  double* result_ptr = result.get_ptr();
  matrix receive_buffer((n_rows + n_cols), n_processors);
  double* receive_buf = receive_buffer.get_ptr();
  size_t i;
  for (i = 0; i < n_rows; i++) {
    appended_vecs[i] = a_ptr[i];
  }
  for (i = 0; i < n_cols; i++) {
    appended_vecs[n_rows + i] = b_ptr[i];
  }

  // TODO: Handle case when not all processes compute

  // allgather round
  mpi_timer(
      MPI_Allgather, appended_vecs, appended_vec_size, MPI_DOUBLE, receive_buf, appended_vec_size, MPI_DOUBLE, comm);
  size_t row_idx_upper = my_start_row + my_n_rows;
  size_t row_i = 0;
  size_t col_i = 0;
  size_t proc_i = 0;
  size_t a_base = 0;
  size_t b_base = 0;
  for (proc_i = 0; proc_i < n_processors; proc_i++) {
    a_base = proc_i * appended_vec_size;
    b_base = proc_i * appended_vec_size + n_rows;
    for (row_i = my_start_row; row_i < row_idx_upper; row_i++) {
      for (col_i = 0; col_i < n_cols; col_i++) {
        result_ptr[row_i * n_cols + col_i] += receive_buf[a_base + row_i] * receive_buf[b_base + col_i];
      }
    }
  }
  //  fprintf(stderr,
  //      "%d [Special Last Block] my_start_row=%zu, my_n_rows=%zu, n_rows=%zu, n_cols=%zu, last_chunk_start_row=%zu, "
  //      "residual_rows=%zu, last_n_rows=%zu\n",
  //      rank, my_start_row, my_n_rows, n_rows, n_cols, last_chunk_start_row, residual_rows, last_n_rows);

  if (special_last_block) {
    // second allgather round matrix prefix
    mpi_timer(MPI_Allgather, &result_ptr[my_start_row * n_cols], my_block_size, MPI_DOUBLE, result_ptr, my_block_size,
        MPI_DOUBLE, comm);
    // last process broadcasts residual chunk to all processes --> last processor is the root of this broadcast
    mpi_timer(MPI_Bcast, &result_ptr[last_chunk_start_row * n_cols], (residual_rows * n_cols), MPI_DOUBLE,
        (num_procs - 1), comm);
  } else {
    // second allgather round
    mpi_timer(MPI_Allgather, &result_ptr[my_start_row * n_cols], my_block_size, MPI_DOUBLE, result_ptr, my_block_size,
        MPI_DOUBLE, comm);
  }

  //  auto result_str = result.string();
  //  auto appended_vec_str = appended_vectors.string();
  //  auto receive_buffer_str = receive_buffer.string();
  //  fprintf(stderr, "%d: Current State:\n Sending-Vectors = %s\n Result = %s\n...\n", rank, appended_vec_str.c_str(),
  //      result_str.c_str());

  //  fprintf(stderr, "%d: Current State:\n Receive buffer = %s\n Sending-Vectors = %s\n Result = %s\n...\n", rank,
  //      receive_buffer_str.c_str(), appended_vec_str.c_str(), result_str.c_str());
}
} // namespace impls::grabenseifner_allgather
