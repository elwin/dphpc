#include "grabenseifner_allgather_scatter/impl.hpp"

#include <math.h>

#include <cmath>

namespace impls::grabenseifner_allgather_scatter {

/**
 * Generalized Rabenseifner using two allgather rounds. The first allgather round is split into an allgather and a
 * all-scatter round, to only send the important parts of the vector a to the individual processes.
 */
void grabenseifner_allgather_scatter::compute(
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
  int i_compute = 1;
  if (n_rows < n_processors) {
    my_n_rows = 1;
    my_start_row = rank;
    if (my_rank >= n_rows) {
      i_compute = 0;
    }
  }
  bool special_last_block = (my_n_rows != last_n_rows);
  // send and receive buffers -->
  // to avoid two broadcast rounds --> append vectors a, b
  const double* a_ptr = a.data();
  const double* b_ptr = b.data();
  double* result_ptr = result.get_ptr();
  matrix receive_buffer_a(my_n_rows, n_processors);
  matrix receive_buffer_b(n_cols, n_processors);
  double* receive_buf_a = receive_buffer_a.get_ptr();
  double* receive_buf_b = receive_buffer_b.get_ptr();
  // TODO: Maybe don't initialize for all other processes (only for last one)
  matrix residual_buffer(residual_rows, n_processors);
  double* residual_buf = residual_buffer.get_ptr();

  // TODO: Handle case when not all processes compute --> should we do this?

  // allgather round for vectors b
  mpi_timer(MPI_Allgather, b_ptr, n_cols, MPI_DOUBLE, receive_buf_b, n_cols, MPI_DOUBLE, comm);
  // num_procs round of MPI_Scatter to distribute parts of the vectors b
  for (int i = 0; i < num_procs; i++) {
    // rank i sends its vectors to all other processes
    mpi_timer(MPI_Scatter, a_ptr, my_n_rows, MPI_DOUBLE, &receive_buf_a[i * my_n_rows], my_n_rows, MPI_DOUBLE, i, comm);
    // TODO: Handle residual elements assigned to the last process
    // TODO: make MPI_IScatter
  }
  // send the residual elements to the last processor
  if (residual_rows != 0) {
    mpi_timer(MPI_Gather, &a_ptr[n_processors * my_n_rows], residual_rows, MPI_DOUBLE, residual_buf, residual_rows,
        MPI_DOUBLE, (num_procs - 1), comm);
  }

  // TODO: on i-scatter --> wait for all to finish in the end

  //  size_t row_idx_upper = my_start_row + my_n_rows;
  size_t row_i = 0;
  size_t col_i = 0;
  size_t proc_i = 0;
  size_t a_base = 0;
  size_t b_base = 0;
  for (proc_i = 0; proc_i < n_processors; proc_i++) {
    a_base = proc_i * my_n_rows;
    b_base = proc_i * n_cols;
    for (row_i = 0; row_i < my_n_rows; row_i++) {
      for (col_i = 0; col_i < n_cols; col_i++) {
        result_ptr[(my_start_row + row_i) * n_cols + col_i] +=
            receive_buf_a[a_base + row_i] * receive_buf_b[b_base + col_i];
      }
    }
  }
  // fix up the last block caused by the residual rows
  if ((residual_rows != 0) && (rank == num_procs - 1)) {
    size_t start_row = n_processors * my_n_rows;
    for (proc_i = 0; proc_i < n_processors; proc_i++) {
      a_base = proc_i * residual_rows;
      b_base = proc_i * n_cols;
      for (row_i = 0; row_i < residual_rows; row_i++) {
        for (col_i = 0; col_i < n_cols; col_i++) {
          result_ptr[(start_row + row_i) * n_cols + col_i] +=
              residual_buf[a_base + row_i] * receive_buf_b[b_base + col_i];
        }
      }
    }
  }

  //  fprintf(stderr,
  //      "%d [Special Last Block] my_start_row=%zu, my_n_rows=%zu, n_rows=%zu, n_cols=%zu, last_chunk_start_row=%zu, "
  //      "residual_rows=%zu, last_n_rows=%zu\nReceived-Buffer-a: %s\n",
  //      rank, my_start_row, my_n_rows, n_rows, n_cols, last_chunk_start_row, residual_rows, last_n_rows,
  //      receive_buffer_a.string().c_str());
  //  if (residual_buf != NULL) {
  //    fprintf(stderr, "%d: RESIDUAL BUFFER: %s", rank, residual_buffer.string().c_str());
  //  }

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
} // namespace impls::grabenseifner_allgather_scatter
