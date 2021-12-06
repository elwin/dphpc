#include "grabenseifner_allgather_segmented/impl.hpp"

#include <math.h>

#include <cmath>

namespace impls::grabenseifner_allgather_segmented {

/**
 * Generalized Rabenseifner using two allgather rounds
 */
void grabenseifner_allgather_segmented::compute(
    const std::vector<vector>& a_in, const std::vector<vector>& b_in, matrix& result) {
  const auto& a = a_in[rank];
  const auto& b = b_in[rank];

  // Check if number of processes assumption true
  if (num_procs < 1) {
    fprintf(stderr, "%d: Starting Generalized Rabenseifner numprocs=%d\n", rank, num_procs);
    return;
  }
  int n_processors = num_procs;
  int n_rows = (int)a.size();
  int n_cols = (int)b.size();
  int my_rank = (int)rank;
  // partition the rows of the final matrix among all processes
  // If there are not enough rows --> divide rows among first n_rows processes
  int my_n_rows = n_rows / n_processors;
  int last_n_rows = n_rows - ((n_processors - 1) * my_n_rows);
  int residual_rows = last_n_rows - my_n_rows;
  int last_chunk_start_row = n_processors * my_n_rows;
  int my_start_row = my_n_rows * rank;
  int my_block_size = my_n_rows * n_cols;
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
  int appended_vec_size = n_rows + n_cols;
  vector appended_vectors(appended_vec_size);
  double* appended_vecs = appended_vectors.data();
  const double* a_ptr = a.data();
  const double* b_ptr = b.data();
  double* result_ptr = result.get_ptr();
  matrix receive_buffer((n_rows + n_cols), n_processors);
  double* receive_buf = receive_buffer.get_ptr();
  int i;
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
  const int GRABENSEIFNER_BSIZE = 16;
  int row_idx_upper = my_start_row + my_n_rows;
  int row_idx_upper_blkd = row_idx_upper - GRABENSEIFNER_BSIZE;
  int n_cols_blkd = n_cols - GRABENSEIFNER_BSIZE;
  int row_i = 0;
  int col_i = 0;
  int row_ii, col_ii;
  int proc_i = 0;
  int a_base = 0;
  int b_base = 0;
  int row_offset = my_start_row * n_cols;
  double a_val, b_val, res_val;

  row_i = my_start_row;
  for (; row_i < row_idx_upper_blkd; row_i += GRABENSEIFNER_BSIZE) {
    for (col_i = 0; col_i < n_cols_blkd; col_i += GRABENSEIFNER_BSIZE) {
      for (row_ii = row_i; row_ii < row_i + GRABENSEIFNER_BSIZE; row_ii++) {
        row_offset = row_ii * n_cols;
        for (col_ii = col_i; col_ii < col_i + GRABENSEIFNER_BSIZE; col_ii++) {
          a_base = 0;
          b_base = n_rows;
          for (proc_i = 0; proc_i < num_procs; proc_i++) {
            a_val = receive_buf[a_base + row_ii];
            b_val = receive_buf[b_base + col_ii];
            res_val = result_ptr[row_offset + col_ii];
            res_val += a_val * b_val;
            result_ptr[row_offset + col_ii] = res_val;
            a_base += appended_vec_size;
            b_base += appended_vec_size;
          }
        }
      }
    }
    // clean up columns
    for (; col_i < n_cols; col_i++) {
      row_offset = row_i * n_cols;
      for (row_ii = row_i; row_ii < row_i + GRABENSEIFNER_BSIZE; row_ii++) {
        a_base = 0;
        b_base = n_rows;
        for (proc_i = 0; proc_i < num_procs; proc_i++) {
          a_val = receive_buf[a_base + row_ii];
          b_val = receive_buf[b_base + col_i];
          res_val = result_ptr[row_offset + col_i];
          res_val += a_val * b_val;
          result_ptr[row_offset + col_i] = res_val;
          a_base += appended_vec_size;
          b_base += appended_vec_size;
        }
        row_offset += n_cols;
      }
    }
  }
  // clean up rows
  row_offset = row_i * n_cols;
  for (; row_i < row_idx_upper; row_i++) {
    for (col_i = 0; col_i < n_cols; col_i++) {
      a_base = 0;
      b_base = n_rows;
      for (proc_i = 0; proc_i < num_procs; proc_i++) {
        a_val = receive_buf[a_base + row_i];
        b_val = receive_buf[b_base + col_i];
        res_val = result_ptr[row_offset + col_i];
        res_val += a_val * b_val;
        result_ptr[row_offset + col_i] = res_val;
        a_base += appended_vec_size;
        b_base += appended_vec_size;
      }
    }
    row_offset += n_cols;
  }

  //      for (row_i = my_start_row; row_i < row_idx_upper; row_i++) {
  //        for (col_i = 0; col_i < n_cols; col_i++) {
  //          a_base = 0;
  //          b_base = n_rows;
  //          for (proc_i = 0; proc_i < num_procs; proc_i++) {
  //            a_val = receive_buf[a_base + row_i];
  //            b_val = receive_buf[b_base + col_i];
  //            res_val = result_ptr[row_offset + col_i];
  //            res_val += a_val * b_val;
  //            result_ptr[row_offset + col_i] = res_val;
  //            a_base += appended_vec_size;
  //            b_base += appended_vec_size;
  //          }
  //        }
  //        row_offset += n_cols;
  //      }

  //  for (proc_i = 0; proc_i < n_processors; proc_i++) {
  //    a_base = proc_i * appended_vec_size;
  //    b_base = proc_i * appended_vec_size + n_rows;
  //    for (row_i = my_start_row; row_i < row_idx_upper; row_i++) {
  //      for (col_i = 0; col_i < n_cols; col_i++) {
  //        result_ptr[row_i * n_cols + col_i] += receive_buf[a_base + row_i] * receive_buf[b_base + col_i];
  //      }
  //    }
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
}
} // namespace impls::grabenseifner_allgather_segmented
