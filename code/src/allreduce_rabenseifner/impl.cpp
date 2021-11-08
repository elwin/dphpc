#include "allreduce_butterfly/impl.hpp"

namespace impls::allreduce_rabenseifner {

/**
 * Performs allreduce according to the rabenseifner algorithm.
 *
 * A first round is composed of a reduce-scatter, distributing parts of the whole matrix across all processes.
 * The second round is composed of adding these parts of the final matrix together.
 *
 * Use MPI_Bsend, because on return of function, the data is buffered, and I can modify the data.
 */
void allreduce_rabenseifner::compute(const std::vector<vector>& a_in, const std::vector<vector>& b_in, matrix& result) {
  const auto& a = a_in[rank];
  const auto& b = b_in[rank];

  // Check if number of processes assumption true
  if (num_procs == 0) {
    fprintf(stderr, "%d: Starting Butterfly numprocs=%d\n", rank, num_procs);
    return;
  }
  int n_rounds = 0;
  int n_processors = num_procs;
  while (n_processors >>= 1) ++n_rounds;
  if (num_procs != (1 << n_rounds)) {
    fprintf(stderr, "%d: Starting Butterfly with non-power-of-2 numprocs=%d\n", rank, num_procs);
    return;
  }
  //  fprintf(stderr, "%d: Starting Butterfly numprocs=%d, n_rounds=%d\n", rank, num_procs, n_rounds);

  int round, recv_rank;
  int one = 1UL;
  int matrix_size = N * M;
  MPI_Status status;

  // Initial Temporary Matrix
  auto tempMatrix = matrix::outer(a, b);

  double* receivedMatrixPtr = new double[N * M];

  double* tempMatrixPtr = tempMatrix.get_ptr();
  for (round = 0; round < n_rounds; round++) {
    // receiver rank (from who we should expect data), is the same rank we send data to
    int bit_vec = (one << round);
    recv_rank = rank ^ bit_vec;

    //    fprintf(stderr, "Process-%d: round=%d, receiver rank=%d, bit-vec=%d\n", rank, round, recv_rank, bit_vec);

    // change ordering to avoid deadlock for larger message sizes
    if (rank < recv_rank) {
      // send
      mpi_timer(MPI_Send, tempMatrixPtr, matrix_size, MPI_DOUBLE, recv_rank, TAG_ALLREDUCE_RABENSEIFNER, comm);
      // receive
      mpi_timer(
          MPI_Recv, receivedMatrixPtr, matrix_size, MPI_DOUBLE, recv_rank, TAG_ALLREDUCE_RABENSEIFNER, comm, &status);
    } else {
      // receive
      mpi_timer(
          MPI_Recv, receivedMatrixPtr, matrix_size, MPI_DOUBLE, recv_rank, TAG_ALLREDUCE_RABENSEIFNER, comm, &status);
      // send
      mpi_timer(MPI_Send, tempMatrixPtr, matrix_size, MPI_DOUBLE, recv_rank, TAG_ALLREDUCE_RABENSEIFNER, comm);
    }

    //    fprintf(stderr, "Process-%d: RECEIVED DATA round=%d, receiver rank=%d\n", rank, round, recv_rank);

    // add received data to the  temporary matrix
    for (int i = 0; i < matrix_size; i++) {
      tempMatrixPtr[i] += receivedMatrixPtr[i];
    }
  }
  delete[] receivedMatrixPtr;
  // Write back temporary matrix to the result matrix
  double* resultPtr = result.get_ptr();
  for (int i = 0; i < matrix_size; i++) {
    resultPtr[i] = tempMatrixPtr[i];
  }
}

} // namespace impls::allreduce_butterfly
