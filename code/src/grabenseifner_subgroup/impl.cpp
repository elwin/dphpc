#include "grabenseifner_subgroup/impl.hpp"

#include <math.h>

#include <cmath>

namespace impls::grabenseifner_subgroup {

// Default number of subgroups (overwritten in main)
int SUBGROUP_N_GROUPS = 4;

/**
 * G-Rabenseifner-Subgroup tries to evaluate a tradeoff between communication time and communication.
 * The algorithm divides the processes into subgroups based on their rank (ignoring any kind of network topology).
 * The subgroups divide the final problem under themselves and the problem is only solved locally in the local group.
 *
 * The initial phase corresponds to an allgather, such that every process (in all subgroups) have all the vectors.
 * The second phase is computing the final matrix in the subgroup.
 *
 * Here we evaluate a similar approach as in the generalized rabenseifner, where the rows are partioned among the
 * processes in the subgroup.
 *
 * NOTE: This algorithm assumes there are more processors in each subgroup than rows in the matrix!
 */
void grabenseifner_subgroup::compute(const std::vector<vector>& a_in, const std::vector<vector>& b_in, matrix& result) {
  const auto& a = a_in[rank];
  const auto& b = b_in[rank];

  // Change the number of groups here
  int N_GROUPS = impls::grabenseifner_subgroup::SUBGROUP_N_GROUPS;

  // Check if number of processes assumption true
  if (num_procs < 1) {
    fprintf(stderr, "%d: Starting Generalized Rabenseifner numprocs=%d\n", rank, num_procs);
    return;
  }
  // for handling github-CI --> if the number of groups is larger than the number of processes
  // --> choose N_GROUPS = num_procs, ie. implement allgather, and each process computes individually
  if (N_GROUPS > num_procs || N_GROUPS < 1) {
    N_GROUPS = num_procs;
  }

  // build the communicators for the individual groups
  int color = rank % N_GROUPS;

  MPI_Comm subgroup;
  MPI_Comm_split(comm, color, rank, &subgroup);
  int subgroup_rank, subgroup_num_procs;
  MPI_Comm_rank(subgroup, &subgroup_rank);
  MPI_Comm_size(subgroup, &subgroup_num_procs);

  //  fprintf(stderr, "%d/%d: (Subgroup %d) Starting Generalized Rabenseifner numprocs=%d, subgroup-num-procs=%d\n",
  //  rank, subgroup_rank, color, num_procs, subgroup_num_procs);

  // prepare splits for computing later
  int n_rows = (int)a.size();
  int n_cols = (int)b.size();
  // partition the rows of the final matrix among all processes in the subgroup
  // If there are not enough rows --> divide rows among first n_rows processes
  int my_n_rows = n_rows / subgroup_num_procs;
  int last_n_rows = n_rows - ((subgroup_num_procs - 1) * my_n_rows);
  int residual_rows = last_n_rows - my_n_rows;
  int last_chunk_start_row = subgroup_num_procs * my_n_rows;
  int my_start_row = my_n_rows * subgroup_rank;
  int my_block_size = my_n_rows * n_cols;
  //  int i_compute = 1; // TODO
  if (n_rows < subgroup_num_procs) {
    my_n_rows = 1;
    my_start_row = subgroup_rank;
    //    if (my_rank >= n_rows) {
    //      i_compute = 0; // TODO
    //    }
  }
  bool special_last_block = (my_n_rows != last_n_rows);
  if (subgroup_rank == subgroup_num_procs - 1) {
    my_n_rows = last_n_rows;
  }
  // send and receive buffers, to avoid two broadcast rounds --> append vectors a, b
  int appended_vec_size = n_rows + n_cols;
  vector appended_vectors(appended_vec_size);
  double* appended_vecs = appended_vectors.data();
  const double* a_ptr = a.data();
  const double* b_ptr = b.data();
  double* result_ptr = result.get_ptr();
  matrix receive_buffer((n_rows + n_cols), num_procs);
  double* receive_buf = receive_buffer.get_ptr();
  int i;
  for (i = 0; i < n_rows; i++) {
    appended_vecs[i] = a_ptr[i];
  }
  for (i = 0; i < n_cols; i++) {
    appended_vecs[n_rows + i] = b_ptr[i];
  }

  // TODO: Handle case when not all processes compute

  // allgather round (between all processes)
  mpi_timer(
      MPI_Allgather, appended_vecs, appended_vec_size, MPI_DOUBLE, receive_buf, appended_vec_size, MPI_DOUBLE, comm);

  // start computing the local problem
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

  //  for (row_i = my_start_row; row_i < row_idx_upper; row_i++) {
  //    for (col_i = 0; col_i < n_cols; col_i++) {
  //      a_base = 0;
  //      b_base = n_rows;
  //      for (proc_i = 0; proc_i < num_procs; proc_i++) {
  //        a_val = receive_buf[a_base + row_i];
  //        b_val = receive_buf[b_base + col_i];
  //        res_val = result_ptr[row_offset + col_i];
  //        res_val += a_val * b_val;
  //        result_ptr[row_offset + col_i] = res_val;
  //        a_base += appended_vec_size;
  //        b_base += appended_vec_size;
  //      }
  //    }
  //    row_offset += n_cols;
  //  }

  //  for (proc_i = 0; proc_i < n_processors; proc_i++) {
  //    a_base = proc_i * appended_vec_size;
  //    b_base = proc_i * appended_vec_size + n_rows;
  //    for (row_i = my_start_row; row_i < row_idx_upper; row_i++) {
  //      for (col_i = 0; col_i < n_cols; col_i++) {
  //        result_ptr[row_i * n_cols + col_i] += receive_buf[a_base + row_i] * receive_buf[b_base + col_i];
  //      }
  //    }
  //  }

  // allgather round inside the subgroup
  if (special_last_block) {
    // second allgather round matrix prefix
    mpi_timer(MPI_Allgather, &result_ptr[my_start_row * n_cols], my_block_size, MPI_DOUBLE, result_ptr, my_block_size,
        MPI_DOUBLE, subgroup);
    // last process broadcasts residual chunk to all processes --> last processor is the root of this broadcast
    mpi_timer(MPI_Bcast, &result_ptr[last_chunk_start_row * n_cols], (residual_rows * n_cols), MPI_DOUBLE,
        (subgroup_num_procs - 1), subgroup);
  } else {
    // second allgather round
    mpi_timer(MPI_Allgather, &result_ptr[my_start_row * n_cols], my_block_size, MPI_DOUBLE, result_ptr, my_block_size,
        MPI_DOUBLE, subgroup);
  }
}

} // namespace impls::grabenseifner_subgroup
