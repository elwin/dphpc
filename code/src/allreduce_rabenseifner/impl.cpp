#include "allreduce_rabenseifner/impl.hpp"

#include <string> // DELETE

namespace impls::allreduce_rabenseifner {

/**
 * Performs allreduce according to the rabenseifner algorithm.
 *
 * A first round is composed of a reduce-scatter, distributing parts of the whole matrix across all processes.
 * The second round is composed of adding these parts of the final matrix together.
 *
 */
void allreduce_rabenseifner::compute(const std::vector<vector>& a_in, const std::vector<vector>& b_in, matrix& result) {
  const auto& a = a_in[rank];
  const auto& b = b_in[rank];

  // Check if number of processes assumption true
  if (num_procs == 0) {
    fprintf(stderr, "%d: Starting Rabenseifner numprocs=%d\n", rank, num_procs);
    return;
  }
  int n_rounds = 0;
  int n_processors = num_procs;
  while (n_processors >>= 1) ++n_rounds;
  int power_2_ranks = (1 << n_rounds);

  // TODO: Extend Rabenseifner with an extension for non-power-of-2 processes
  if (num_procs != (1 << n_rounds)) {
    fprintf(stderr, "%d: Starting Rabenseifner with non-power-of-2 numprocs=%d\n", rank, num_procs);
    return;
  }

  int round, recv_rank;
  int one = 1UL;
  int matrix_size = N * M;
  MPI_Status status;

  // Initial Temporary Matrix
  result.set_outer_product(a, b);
  double* tempMatrixPtr = result.get_ptr();
  double* receivedMatrixPtr = new double[N * M];

  // [RABENSEIFNER INDICES]
  // Indices list: Represents a partition of the N*M matrix in terms of indices. Upper index is always exclusive.
  std::vector<int> all_indices(power_2_ranks + 1, 0);
  all_indices[0] = 0;
  all_indices[power_2_ranks] = matrix_size;
  int chunk_size = matrix_size / power_2_ranks;
  for (int i = 1; i < power_2_ranks; i++) {
    all_indices[i] = all_indices[i - 1] + chunk_size;
  }
  // My Indices for individual rounds (symmetric --> used once forward, once backward)
  std::vector<int> my_send_indices_lower(n_rounds, 0);
  std::vector<int> my_send_indices_upper(n_rounds, 0);
  std::vector<int> my_recv_indices_lower(n_rounds, 0);
  std::vector<int> my_recv_indices_upper(n_rounds, 0);
  int current_idx_lower = 0;
  int current_idx_upper = power_2_ranks;
  int middle_idx;
  for (round = n_rounds - 1; round >= 0; round--) {
    //  for (round = 0; round < n_rounds; round++) {
    middle_idx = ((current_idx_lower + current_idx_upper + 1) / 2);
    if (rank < middle_idx) {
      my_recv_indices_lower[round] = current_idx_lower;
      my_recv_indices_upper[round] = middle_idx;
      my_send_indices_lower[round] = middle_idx;
      my_send_indices_upper[round] = current_idx_upper;

      current_idx_upper = middle_idx;
    } else {
      my_send_indices_lower[round] = current_idx_lower;
      my_send_indices_upper[round] = middle_idx;
      my_recv_indices_lower[round] = middle_idx;
      my_recv_indices_upper[round] = current_idx_upper;

      current_idx_lower = middle_idx;
    }
  }

  //  std::string string_vector;                              // DELETE
  //  for (int i = 0; i <= power_2_ranks; i++) {              // DELETE
  //    string_vector.append(std::to_string(all_indices[i])); // DELETE
  //    string_vector.append(", ");
  //  }
  //  std::string send_indices_lower;                                        // DELETE
  //  std::string send_indices_upper;                                        // DELETE
  //  std::string recv_indices_lower;                                        // DELETE
  //  std::string recv_indices_upper;                                        // DELETE
  //  for (int i = 0; i < n_rounds; i++) {                                   // DELETE
  //    send_indices_lower.append(std::to_string(my_send_indices_lower[i])); // DELETE
  //    send_indices_upper.append(std::to_string(my_send_indices_upper[i])); // DELETE
  //    recv_indices_lower.append(std::to_string(my_recv_indices_lower[i])); // DELETE
  //    recv_indices_upper.append(std::to_string(my_recv_indices_upper[i])); // DELETE
  //    send_indices_lower.append(", ");
  //    send_indices_upper.append(", ");
  //    recv_indices_lower.append(", ");
  //    recv_indices_upper.append(", ");
  //  }
  //  fprintf(stderr, "%d: Send Lower Indices=%s\n", rank, send_indices_lower.c_str());    // DELETE
  //  fprintf(stderr, "%d: Send Upper Indices=%s\n", rank, send_indices_upper.c_str());    // DELETE
  //  fprintf(stderr, "%d: Receive Lower Indices=%s\n", rank, recv_indices_lower.c_str()); // DELETE
  //  fprintf(stderr, "%d: Receive Upper Indices=%s\n", rank, recv_indices_upper.c_str()); // DELETE

  // TODO: Optimization, in Butterfly-Allgather, directly use the result matrix to avoid duplicate copy (cf. 'TODO
  // (Optimization)')

  // [RABENSEIFNER - BUTTERFLY REDUCE SCATTER]
  // --> send chunks, and add them to your temporary matrix
  int idx_lower_send, idx_upper_send, idx_lower_recv, idx_upper_recv;
  for (round = n_rounds - 1; round >= 0; round--) {
    // receiver rank (from who we should expect data), is the same rank we send data to
    int bit_vec = (one << round);
    recv_rank = rank ^ bit_vec;

    idx_lower_send = all_indices[my_send_indices_lower[round]];
    idx_upper_send = all_indices[my_send_indices_upper[round]];
    idx_lower_recv = all_indices[my_recv_indices_lower[round]];
    idx_upper_recv = all_indices[my_recv_indices_upper[round]];
    int chunk_size_send = idx_upper_send - idx_lower_send;
    int chunk_size_recv = idx_upper_recv - idx_lower_recv;

    //    fprintf(stderr, "%d: ROUND=%d, Send [%d, %d) to %d, Receive [%d, %d) from %d, Send-Size=%d, Recv-size=%d\n",
    //    rank,
    //        round, idx_lower_send, idx_upper_send, recv_rank, idx_lower_recv, idx_upper_recv, recv_rank,
    //        chunk_size_send, chunk_size_recv); // DELETE

    // TODO: CHECK POINTER ARITHMETIC CORRECTNESS IN SEND

    // ordering to avoid deadlock for larger message sizes
    if (rank < recv_rank) {
      // send
      mpi_timer(MPI_Send, tempMatrixPtr + idx_lower_send, chunk_size_send, MPI_DOUBLE, recv_rank,
          TAG_ALLREDUCE_RABENSEIFNER, comm);
      // receive
      mpi_timer(MPI_Recv, receivedMatrixPtr, chunk_size_recv, MPI_DOUBLE, recv_rank, TAG_ALLREDUCE_RABENSEIFNER, comm,
          &status);
    } else {
      // receive
      mpi_timer(MPI_Recv, receivedMatrixPtr, chunk_size_recv, MPI_DOUBLE, recv_rank, TAG_ALLREDUCE_RABENSEIFNER, comm,
          &status);
      // send
      mpi_timer(MPI_Send, tempMatrixPtr + idx_lower_send, chunk_size_send, MPI_DOUBLE, recv_rank,
          TAG_ALLREDUCE_RABENSEIFNER, comm);
    }

    // add received chunk to the temporary matrix (at the correct place)
    for (int i = 0; i < chunk_size_recv; i++) {
      tempMatrixPtr[idx_lower_recv + i] += receivedMatrixPtr[i];
    }
  }
  // TODO (Optimization): Put my small chunk directly into the result array
  // [RABENSEIFNER - BUTTERFLY ALLGATHER]
  // --> send chunks (final value), and set them directly in the result array
  // --> reverse send and receive indices!!!
  for (round = 0; round < n_rounds; round++) {
    //    fprintf(stderr, "%d: ROUND=%d\n", rank, round); // DELETE
    // receiver rank (from who we should expect data), is the same rank we send data to
    int bit_vec = (one << round);
    recv_rank = rank ^ bit_vec;

    // Note: send and receive need to be reversed here!
    idx_lower_send = all_indices[my_recv_indices_lower[round]];
    idx_upper_send = all_indices[my_recv_indices_upper[round]];
    idx_lower_recv = all_indices[my_send_indices_lower[round]];
    idx_upper_recv = all_indices[my_send_indices_upper[round]];
    int chunk_size_send = idx_upper_send - idx_lower_send;
    int chunk_size_recv = idx_upper_recv - idx_lower_recv;

    //    fprintf(stderr,
    //        "%d: [GATHER] ROUND=%d, Send [%d, %d) to %d, Receive [%d, %d) from %d, Send-Size=%d, Recv-size=%d\n",
    //        rank, round, idx_lower_send, idx_upper_send, recv_rank, idx_lower_recv, idx_upper_recv, recv_rank,
    //        chunk_size_send, chunk_size_recv); // DELETE

    // TODO (Optimization): Send directly from result array, and write directly into result array upon receive
    //  --> make use of MPI to pass the correct pointer
    // ordering to avoid deadlock for larger message sizes
    if (rank < recv_rank) {
      // send
      mpi_timer(MPI_Send, tempMatrixPtr + idx_lower_send, chunk_size_send, MPI_DOUBLE, recv_rank,
          TAG_ALLREDUCE_RABENSEIFNER, comm);
      // receive
      mpi_timer(MPI_Recv, receivedMatrixPtr, chunk_size_recv, MPI_DOUBLE, recv_rank, TAG_ALLREDUCE_RABENSEIFNER, comm,
          &status);
    } else {
      // receive
      mpi_timer(MPI_Recv, receivedMatrixPtr, chunk_size_recv, MPI_DOUBLE, recv_rank, TAG_ALLREDUCE_RABENSEIFNER, comm,
          &status);
      // send
      mpi_timer(MPI_Send, tempMatrixPtr + idx_lower_send, chunk_size_send, MPI_DOUBLE, recv_rank,
          TAG_ALLREDUCE_RABENSEIFNER, comm);
    }

    // TODO (Optimization): Delete this copying, once MPI writes chunks directly into result matrix
    // write values into the resulting array (just setting, no adding)
    for (int i = 0; i < chunk_size_recv; i++) {
      tempMatrixPtr[idx_lower_recv + i] = receivedMatrixPtr[i];
    }
  }
  delete[] receivedMatrixPtr;
}

} // namespace impls::allreduce_rabenseifner
