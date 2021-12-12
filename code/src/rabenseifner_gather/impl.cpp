#include "rabenseifner_gather/impl.hpp"

#include <string> // DELETE

namespace impls::rabenseifner_gather {

/**
 * Performs a variant of the rabenseifner algorithm adapted to our problem statement of computing the outer product of
 * two vectors and summing over multiple such vectors.
 *
 * A first round is composed of a gather-round where every process sends vectors and parts of vectors to processes that
 * need them to compute their sub-problem.
 * This gather step allows for an easy optimization of reusing the copying of contiguous memory by the MPI function.
 * Therefore this implementation lends itself to partition the rows of the final matrix to be partitioned among the
 * individual workers.
 *
 * The second round uses a butterfly-allgather mechanism to distribute the computed rows for every process to have the
 * final matrix.
 *
 * For non-powers-of-2 processes, only next smaller power-of-2 processes actually compute. During the gather-step of the
 * vectors, all agents send their vectors to the responsible worker, regardless of whether they actually take part
 * during the computation or not.
 *
 */
void rabenseifner_gather::compute(const std::vector<vector>& a_in, const std::vector<vector>& b_in, matrix& result) {
  auto& a = a_in[rank];
  auto& b = b_in[rank];

  // Check if number of processes assumption true
  if (num_procs < 1) {
    fprintf(stderr, "%d: Starting Rabenseifner numprocs=%d\n", rank, num_procs);
    return;
  }
  int n_rounds = 0;
  int n_processors = num_procs;
  while (n_processors >>= 1) ++n_rounds;
  int power_2_ranks = (1 << n_rounds);

  // TODO: Extend Rabenseifner-Gather with an extension for non-power-of-2 processes
  if (num_procs != (1 << n_rounds)) {
    throw std::runtime_error(
        "Starting Rabenseifner-Gather with non-power-of-2 processes, numprocs=" + std::to_string(num_procs));
  }
  if (M < num_procs) {
    throw std::runtime_error("Starting Rabenseifner-Gather more processes (" + std::to_string(num_procs) +
                             ") than rows (b.size() = " + std::to_string(result.rows) + ")");
  }

  int round, recv_rank;
  int one = 1UL;
  int matrix_size = N * M;
  int n_rows = (int)a.size();
  int n_cols = (int)b.size();
  MPI_Status status;

  // Partition the rows among the processes
  int my_n_rows = n_rows / power_2_ranks;                       // divide rows evenly among workers
  int last_n_rows = n_rows - ((power_2_ranks - 1) * my_n_rows); // last process finishes up rows
  int recv_a_elements = my_n_rows;
  if (last_n_rows > my_n_rows) {
    recv_a_elements = last_n_rows;
  }
  int my_result_row_offset = rank * my_n_rows;

  std::vector<MPI_Request> send_reqs;
  MPI_Request request;
  vector recv_vec_A(recv_a_elements);
  vector recv_vec_B(n_cols);
  double* recv_vec_A_ptr = recv_vec_A.data();
  double* recv_vec_B_ptr = recv_vec_B.data(); // make buffer large enough
  double* resultPtr = result.get_ptr();

  // [VECTOR-GATHER STAGE]
  // Send vectors
  int send_n_rows = my_n_rows;
  for (int i = 0; i < power_2_ranks; i++) {
    if (i == rank) {
      continue;
    }
    if (i == (power_2_ranks - 1)) {
      send_n_rows = last_n_rows;
    }
    // send full vector a
    send_reqs.push_back(nullptr);
    mpi_timer(MPI_Isend, &a[i * my_n_rows], send_n_rows, MPI_DOUBLE, i, TAG_RABENSEIFNER_GATHER_VEC_A, comm,
        &send_reqs.back());
    // send part of vector b
    send_reqs.push_back(nullptr);
    mpi_timer(MPI_Isend, &b[0], n_cols, MPI_DOUBLE, i, TAG_RABENSEIFNER_GATHER_VEC_B, comm, &send_reqs.back());
  }

  // Receive and process vectors
  int recv_n_rows = my_n_rows;
  if (rank == (power_2_ranks - 1)) {
    recv_n_rows = last_n_rows;
  }
  for (int i = 0; i < power_2_ranks; i++) {
    if (i == rank) {
      // copy parts of b into the subvector and use it to compute the outer product
      vector a_subvec(recv_n_rows);
      for (int j = 0; j < recv_n_rows; j++) {
        a_subvec[j] = a[rank * my_n_rows + j];
      }
      result.add_submatrix_outer_product(my_result_row_offset, 0, a_subvec, b);
      continue;
    }
    recv_vec_A.resize(recv_n_rows, 0.0);
    recv_vec_A_ptr = recv_vec_A.data();
    // receive both vectors
    mpi_timer(MPI_Recv, recv_vec_A_ptr, recv_n_rows, MPI_DOUBLE, i, TAG_RABENSEIFNER_GATHER_VEC_A, comm, &status);
    mpi_timer(MPI_Recv, recv_vec_B_ptr, n_cols, MPI_DOUBLE, i, TAG_RABENSEIFNER_GATHER_VEC_B, comm, &status);

    vector received_A(recv_vec_A_ptr, recv_vec_A_ptr + recv_n_rows);
    vector received_B(recv_vec_B_ptr, recv_vec_B_ptr + n_cols);

    // add the outer product to the current result matrix
    result.add_submatrix_outer_product(my_result_row_offset, 0, recv_vec_A, recv_vec_B);
  }

  // TODO: Cleanup index computation
  // [BUTTERFLY-SCATTER]
  // Construct Indices for Butterfly-Scatter
  // Indices list: Represents a partition of the N*M matrix in terms of indices. Upper index is always exclusive.
  std::vector<int> all_indices(power_2_ranks + 1, 0);
  all_indices[0] = 0;
  all_indices[power_2_ranks] = matrix_size;
  int chunk_size = my_n_rows * n_cols;
  for (int i = 1; i < power_2_ranks; i++) {
    all_indices[i] = all_indices[i - 1] + chunk_size;
  }
  // My Indices for individual rounds
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
      my_send_indices_lower[round] = current_idx_lower;
      my_send_indices_upper[round] = middle_idx;
      my_recv_indices_lower[round] = middle_idx;
      my_recv_indices_upper[round] = current_idx_upper;

      current_idx_upper = middle_idx;
    } else {
      my_recv_indices_lower[round] = current_idx_lower;
      my_recv_indices_upper[round] = middle_idx;
      my_send_indices_lower[round] = middle_idx;
      my_send_indices_upper[round] = current_idx_upper;

      current_idx_lower = middle_idx;
    }
  }
  int idx_lower_send, idx_upper_send, idx_lower_recv, idx_upper_recv;
  for (round = 0; round < n_rounds; round++) {
    // receiver rank (from who we should expect data), is the same rank we send data to
    int bit_vec = (one << round);
    recv_rank = rank ^ bit_vec;

    // Note: send and receive need to be reversed here!
    idx_lower_send = all_indices[my_send_indices_lower[round]];
    idx_upper_send = all_indices[my_send_indices_upper[round]];
    idx_lower_recv = all_indices[my_recv_indices_lower[round]];
    idx_upper_recv = all_indices[my_recv_indices_upper[round]];
    int chunk_size_send = idx_upper_send - idx_lower_send;
    int chunk_size_recv = idx_upper_recv - idx_lower_recv;

    // send, receive, and wait
    mpi_timer(MPI_Isend, resultPtr + idx_lower_send, chunk_size_send, MPI_DOUBLE, recv_rank, TAG_RABENSEIFNER_GATHER,
        comm, &request);
    mpi_timer(MPI_Recv, resultPtr + idx_lower_recv, chunk_size_recv, MPI_DOUBLE, recv_rank, TAG_RABENSEIFNER_GATHER,
        comm, &status);
    MPI_Wait(&request, &status);
  }

  MPI_Waitall(send_reqs.size(), send_reqs.data(), MPI_STATUSES_IGNORE);
}

} // namespace impls::rabenseifner_gather
