#include "allreduce_butterfly/impl.hpp"

namespace impls::allreduce_butterfly {

/**
 * Performs allreduce with explicit butterfly communication.
 *
 * The implementation assumes the number of processes to be a power of 2.
 *
 * Use MPI_Bsend, because on return of function, the data is buffered, and I can modify the data.
 */
void allreduce_butterfly::compute(const std::vector<vector>& a_in, const std::vector<vector>& b_in, matrix& result) {
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

  // Reduce to the nearest power of 2
  // - Extra nodes send their data to the first nodes
  // - normal butterfly
  // - the extra nodes send their local copy back to the extra nodes

  int round, recv_rank;
  int one = 1UL;
  int matrix_size = N * M;
  MPI_Status status;

  // Initial Temporary Matrix
  auto tempMatrix = matrix::outer(a, b);
  double* receivedMatrixPtr = new double[N * M];
  double* tempMatrixPtr = tempMatrix.get_ptr();
  double* resultPtr = result.get_ptr();

  // Initialization for non-power-of-2 setup
  int power_2_ranks = (1 << n_rounds);
  bool non_power_of_2_rounds = (num_procs != power_2_ranks);
  bool i_am_idle_rank = (rank >= power_2_ranks);  // whether I am idle or not
  int n_idle_ranks = num_procs - power_2_ranks;   // number of idle processors
  bool i_am_idle_partner = (rank < n_idle_ranks); // whether I am partner of an idle process
  int idle_partner_rank = 0;
  if (i_am_idle_rank) {
    idle_partner_rank = rank - power_2_ranks;
  } else if (i_am_idle_partner) {
    idle_partner_rank = rank + power_2_ranks;
  }

  // Start Reducing to nearest power of 2
  if (non_power_of_2_rounds && i_am_idle_rank) {
    fprintf(stderr, "%d: [IDLE] Sending to rank=%d\n", rank, idle_partner_rank);
    // send
    mpi_timer(
        MPI_Send, tempMatrixPtr, matrix_size, MPI_DOUBLE, idle_partner_rank, TAG_ALLGATHER_BUTTERFLY_REDUCE, comm);
  }
  if (non_power_of_2_rounds && i_am_idle_partner) {
    fprintf(stderr, "%d: [IDLE-PARTNER] Receiving from rank=%d\n", rank, idle_partner_rank);
    // receive
    mpi_timer(MPI_Recv, receivedMatrixPtr, matrix_size, MPI_DOUBLE, idle_partner_rank, TAG_ALLGATHER_BUTTERFLY_REDUCE,
        comm, &status);
    // add received data to the  temporary matrix
    for (int i = 0; i < matrix_size; i++) {
      tempMatrixPtr[i] += receivedMatrixPtr[i];
    }
  }

  // [START BUTTERFLY ROUNDS]
  for (round = 0; round < n_rounds; round++) {
    // receiver rank (from who we should expect data), is the same rank we send data to
    int bit_vec = (one << round);
    recv_rank = rank ^ bit_vec;

    //    fprintf(stderr, "Process-%d: round=%d, receiver rank=%d, bit-vec=%d\n", rank, round, recv_rank, bit_vec);

    // change ordering to avoid deadlock for larger message sizes
    if (rank < recv_rank) {
      // send
      mpi_timer(MPI_Send, tempMatrixPtr, matrix_size, MPI_DOUBLE, recv_rank, TAG_ALLGATHER_BUTTERFLY, comm);
      // receive
      mpi_timer(
          MPI_Recv, receivedMatrixPtr, matrix_size, MPI_DOUBLE, recv_rank, TAG_ALLGATHER_BUTTERFLY, comm, &status);
    } else {
      // receive
      mpi_timer(
          MPI_Recv, receivedMatrixPtr, matrix_size, MPI_DOUBLE, recv_rank, TAG_ALLGATHER_BUTTERFLY, comm, &status);
      // send
      mpi_timer(MPI_Send, tempMatrixPtr, matrix_size, MPI_DOUBLE, recv_rank, TAG_ALLGATHER_BUTTERFLY, comm);
    }

    //    fprintf(stderr, "Process-%d: RECEIVED DATA round=%d, receiver rank=%d\n", rank, round, recv_rank);

    // add received data to the  temporary matrix
    for (int i = 0; i < matrix_size; i++) {
      tempMatrixPtr[i] += receivedMatrixPtr[i];
    }
  }
  // [END BUTTERFLY ROUNDS]

  // [FINISH Reducing to nearest power of 2]
  if (non_power_of_2_rounds && i_am_idle_partner) {

    fprintf(stderr, "%d: [IDLE-PARTNER] Sending to rank=%d\n", rank, idle_partner_rank);
    // send
    mpi_timer(
        MPI_Send, tempMatrixPtr, matrix_size, MPI_DOUBLE, idle_partner_rank, TAG_ALLGATHER_BUTTERFLY_REDUCE, comm);
  }
  if (non_power_of_2_rounds && i_am_idle_rank) {

    fprintf(stderr, "%d: [IDLE] REceiving from rank=%d\n", rank, idle_partner_rank);
    // receive --> puts results automatically in tempMatrix
    mpi_timer(MPI_Recv, tempMatrixPtr, matrix_size, MPI_DOUBLE, idle_partner_rank, TAG_ALLGATHER_BUTTERFLY_REDUCE,
              comm, &status);
  }

  delete[] receivedMatrixPtr;
  // Write back temporary matrix to the result matrix
  for (int i = 0; i < matrix_size; i++) {
    resultPtr[i] = tempMatrixPtr[i];
  }
}

} // namespace impls::allreduce_butterfly
