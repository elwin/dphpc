#include "grabenseifner_allgather_segmented/impl.hpp"

#include <math.h>

#include <cmath>

#include "grabenseifner_allgather/impl.hpp"

static constexpr int SEG_SIZE = 4096;
static constexpr int SEG_EL = SEG_SIZE / sizeof(double);

namespace impls::grabenseifner_allgather_segmented {

/**
 * Generalized Rabenseifner using two allgather rounds with pipelining
 */
void grabenseifner_allgather_segmented::compute(
    const std::vector<vector>& a_in, const std::vector<vector>& b_in, matrix& result) {
  const auto& a = a_in[rank];
  const auto& b = b_in[rank];

  int n_processors = num_procs;
  int n_rows = (int)a.size();
  int n_cols = (int)b.size();

  int appended_vec_size = n_rows + n_cols;

  /*
   * If the sent vector size is smaller than the number of elements in a segment, we can just as well use the
   * non-segmented implementation
   */
  if (appended_vec_size < SEG_EL) {
    grabenseifner_allgather::grabenseifner_allgather impl(comm, rank, num_procs, N, M);
    impl.compute(a_in, b_in, result);
    return;
  }

  int my_rank = (int)rank;
  // partition the rows of the final matrix among all processes
  // If there are not enough rows --> divide rows among first n_rows processes
  int my_n_rows = n_rows / n_processors;
  // Number of rows the last process is responsible for
  const int last_n_rows = n_rows - ((n_processors - 1) * my_n_rows);
  /*
   * Number of additional rows, the last process has in relation to other processes.
   */
  const int residual_rows = last_n_rows - my_n_rows;
  const int last_chunk_start_row = n_processors * my_n_rows;

  /*
   * Each process is responsible for rows [my_start_row, my_start_row + my_n_rows)
   */
  int my_start_row = my_n_rows * rank;

  /*
   * Number of doubles of the final matrix this process sends in the last
   * allgather round
   */
  const int my_block_size = my_n_rows * n_cols;
  if (n_rows < n_processors) {
    my_n_rows = 1;
    my_start_row = rank;
  }

  bool special_last_block = (my_n_rows != last_n_rows);
  if (my_rank == n_processors - 1) {
    my_n_rows = last_n_rows;
  }
  // send and receive buffers
  // to avoid two broadcast rounds --> append vectors a, b
  vector appended_vectors(appended_vec_size);
  double* appended_vecs = appended_vectors.data();
  const double* a_ptr = a.data();
  const double* b_ptr = b.data();
  double* result_ptr = result.get_ptr();
  std::vector<double> receive_buf(appended_vec_size * n_processors);
  int i;
  for (i = 0; i < n_rows; i++) {
    appended_vecs[i] = a_ptr[i];
  }
  for (i = 0; i < n_cols; i++) {
    appended_vecs[n_rows + i] = b_ptr[i];
  }

  // TODO: Handle case when not all processes compute

  // allgather round
  mpi_timer(MPI_Allgather, appended_vecs, appended_vec_size, MPI_DOUBLE, receive_buf.data(), appended_vec_size,
      MPI_DOUBLE, comm);
  const int row_idx_upper = my_start_row + my_n_rows;
  for (int row_i = my_start_row; row_i < row_idx_upper; row_i++) {
    for (int col_i = 0; col_i < n_cols; col_i++) {
      int a_base = 0;
      int b_base = n_rows;
      for (int proc_i = 0; proc_i < num_procs; proc_i++) {
        double a_val = receive_buf[a_base + row_i];
        double b_val = receive_buf[b_base + col_i];
        result.get(row_i, col_i) += a_val * b_val;
        a_base += appended_vec_size;
        b_base += appended_vec_size;
      }
    }
  }

  if (special_last_block) {
    // second allgather round matrix prefix
    mpi_timer(MPI_Allgather, &result.get(my_start_row, 0), my_block_size, MPI_DOUBLE, result_ptr, my_block_size,
        MPI_DOUBLE, comm);
    // last process broadcasts residual chunk to all processes --> last processor is the root of this broadcast
    mpi_timer(
        MPI_Bcast, &result.get(last_chunk_start_row, 0), (residual_rows * n_cols), MPI_DOUBLE, (num_procs - 1), comm);
  } else {
    // second allgather round
    mpi_timer(MPI_Allgather, &result.get(my_start_row, 0), my_block_size, MPI_DOUBLE, result_ptr, my_block_size,
        MPI_DOUBLE, comm);
  }
}
} // namespace impls::grabenseifner_allgather_segmented
