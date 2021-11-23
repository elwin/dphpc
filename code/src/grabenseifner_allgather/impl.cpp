#include "grabenseifner_allgather/impl.hpp"

#include <math.h>

#include <cmath>

namespace impls::grabenseifner_allgather {

/**
 * Generalized Rabenseifner using two allgather rounds
 */
void grabenseifner_allgather::compute(const std::vector<vector>& a_in, const std::vector<vector>& b_in, matrix& result) {
  const auto& a = a_in[rank];
  const auto& b = b_in[rank];

  fprintf(stderr, "%d: Starting Generalized Rabenseifner numprocs=%d\n", rank, num_procs);
  return;

  // Check if number of processes assumption true
  if (num_procs < 1) {
    fprintf(stderr, "%d: Starting Generalized Rabenseifner numprocs=%d\n", rank, num_procs);
    return;
  }
  int n_processors = num_procs;
  int matrix_size = N * M;
  int n_rows = (int)a.size();
  int n_cols = (int)b.size();
  // partition the rows of the final matrxi among all processes
  // If there are not enough rows --> divide rows among first n_rows processes
  int my_n_rows = n_rows / n_processors;
  int last_n_rows = n_rows - (n_processors * my_n_rows);
  int i_compute = 1;
  if (n_rows < n_processors) {
    my_n_rows = 1;
    last_n_rows = 0;
    if (rank >= n_rows) {
      i_compute = 0;
    }
  }
  // send and receive buffers
  // to avoid two broadcast rounds --> append vectors a, b
  double* appended_vecs = new double[n_rows + n_cols];
  const double* a_ptr = a.data();
  const double* b_ptr = b.data();
  double* receive_buf = new double[(n_rows + n_cols) * n_processors];
  int i;
  for (i = 0; i < n_rows; i++) {
    appended_vecs[i] = a_ptr[i];
  }
  for (i = 0; i < n_cols; i++) {
    appended_vecs[n_rows + i] = b_ptr[i];
  }
  // allgather round
  mpi_timer(MPI_Allgather, appened_vecs, n_rows + n_cols, MPI_DOUBLE, receive_buf, n_rows + n_cols, MPI_DOUBLE, comm);

}
} // namespace impls::grabenseifner_allgather
