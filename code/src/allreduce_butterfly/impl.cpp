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

  int round, recv_rank;
  int one = 1UL;
  int matrix_size = N * M;
  MPI_Status status;
  MPI_Request request;
  // Write initial outer product to result matrix
  set_outer_product(result, a, b);
  double* receivedMatrixPtr = new double[N * M];
  double* resultPtr = result.get_ptr();

  int n_rounds = 0;
  int n_processors = num_procs;
  while (n_processors >>= 1) ++n_rounds;

  // Reduce to the nearest power of 2
  // - Extra nodes send their data to the first nodes
  // - normal butterfly
  // - the extra nodes send their local copy back to the extra nodes

  // Initialization for non-power-of-2 setup
  int power_2_ranks = (1 << n_rounds);
  bool non_power_of_2_rounds = (num_procs != power_2_ranks);
  bool i_am_idle_rank = (rank >= power_2_ranks);                 // whether I am idle or not
  bool i_am_idle_partner = (rank < (num_procs - power_2_ranks)); // whether I am partner of an idle process
  int idle_partner_rank = 0;
  if (i_am_idle_rank) {
    idle_partner_rank = rank - power_2_ranks;
  } else if (i_am_idle_partner) {
    idle_partner_rank = rank + power_2_ranks;
  }

  // Start Reducing to nearest power of 2
  if (non_power_of_2_rounds && i_am_idle_rank) {
    // send
    mpi_timer(MPI_Ssend, resultPtr, matrix_size, MPI_DOUBLE, idle_partner_rank, TAG_ALLREDUCE_BUTTERFLY_REDUCE, comm);
  }
  if (non_power_of_2_rounds && i_am_idle_partner) {
    // receive
    mpi_timer(MPI_Recv, receivedMatrixPtr, matrix_size, MPI_DOUBLE, idle_partner_rank, TAG_ALLREDUCE_BUTTERFLY_REDUCE,
        comm, &status);
    // add received data to the  temporary matrix
    for (int i = 0; i < matrix_size; i++) {
      resultPtr[i] += receivedMatrixPtr[i];
    }
  }

  // [START BUTTERFLY ROUNDS]
  if (!i_am_idle_rank) {
    for (round = 0; round < n_rounds; round++) {
      // receiver rank (from who we should expect data), is the same rank we send data to
      int bit_vec = (one << round);
      recv_rank = rank ^ bit_vec;

      // send
      mpi_timer(MPI_Isend, resultPtr, matrix_size, MPI_DOUBLE, recv_rank, TAG_ALLREDUCE_BUTTERFLY, comm, &request);
      // receive
      mpi_timer(
          MPI_Recv, receivedMatrixPtr, matrix_size, MPI_DOUBLE, recv_rank, TAG_ALLREDUCE_BUTTERFLY, comm, &status);

      // wait for receive --> to use buffer again
      MPI_Wait(&request, MPI_STATUS_IGNORE);

      //    fprintf(stderr, "Process-%d: round=%d, receiver rank=%d, bit-vec=%d\n", rank, round, recv_rank, bit_vec);
      //    fprintf(stderr, "Process-%d: RECEIVED DATA round=%d, receiver rank=%d\n", rank, round, recv_rank);

      // add received data to the  temporary matrix
      for (int i = 0; i < matrix_size; i++) {
        resultPtr[i] += receivedMatrixPtr[i];
      }
    }
  }
  // [END BUTTERFLY ROUNDS]

  // [FINISH Reducing to nearest power of 2]
  if (non_power_of_2_rounds && i_am_idle_partner) {
    //    fprintf(stderr, "%d: [IDLE-PARTNER] Sending to rank=%d\n", rank, idle_partner_rank);
    // send
    mpi_timer(MPI_Isend, resultPtr, matrix_size, MPI_DOUBLE, idle_partner_rank, TAG_ALLREDUCE_BUTTERFLY_REDUCE, comm,
        &request);
    // --> ignore request, since I am finished after this
  }
  if (non_power_of_2_rounds && i_am_idle_rank) {
    //    fprintf(stderr, "%d: [IDLE] Receiving from rank=%d\n", rank, idle_partner_rank);
    // receive --> puts results automatically in tempMatrix
    mpi_timer(
        MPI_Recv, resultPtr, matrix_size, MPI_DOUBLE, idle_partner_rank, TAG_ALLREDUCE_BUTTERFLY_REDUCE, comm, &status);
  }
  delete[] receivedMatrixPtr;
}

} // namespace impls::allreduce_butterfly
