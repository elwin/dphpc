#include "rabenseifner_scatter/impl.hpp"

#include <math.h>

#include <cmath>
#include <string> // DELETE

namespace impls::rabenseifner_scatter {

/**
 * Performs a variant of the rabenseifner algorithm adapted to our problem statement of computing the outer product of
 * two vectors and summing over multiple such vectors. The final matrix is partitioned into equally sized sub-matrices,
 * which are distributed among all worker processes.
 *
 * The first round is composed of a reduce-scatter, distributing parts of the original vectors. At the end of this step,
 * each worker process should have all sub-vectors to compute their sub-matrix.
 *
 * The second step is composed of computing their sub-matrix by computing outer-products of the received sub-vectors and
 * adding them all together.
 *
 * A third round is composed of a butterfly-allgather to distribute all final sub-matrices among all processes.
 *
 * TODO: For non-powers-of-2 processes
 *
 */
void rabenseifner_scatter::compute(const std::vector<vector>& a_in, const std::vector<vector>& b_in, matrix& result) {
  const auto& a = a_in[rank];
  const auto& b = b_in[rank];

  /*// Create data types for rabenseifner scatter
  // TODO: Possibly move this outside this compute function
  MPI_Datatype SubsetVector;
  MPI_Aint displacements[5];
  int lengths[5] = {1, 1, 1, 1, 1000000}; // Would need to make individual types for individual vector sizes?
  struct SubVec dummy_subvec;
  MPI_Aint base_address;
  MPI_Get_address(&dummy_subvec, &base_address);
  MPI_Get_address(&dummy_subvec.n, &displacements[0]);
  MPI_Get_address(&dummy_subvec.start_idx, &displacements[1]);
  MPI_Get_address(&dummy_subvec.end_idx, &displacements[2]);
  MPI_Get_address(&dummy_subvec.owner_rank, &displacements[3]);
  MPI_Get_address(&dummy_subvec.vec , &displacements[4]);
  displacements[0] = MPI_Aint_diff(displacements[0], base_address);
  displacements[1] = MPI_Aint_diff(displacements[1], base_address);
  displacements[2] = MPI_Aint_diff(displacements[2], base_address);
  displacements[3] = MPI_Aint_diff(displacements[3], base_address);
  displacements[4] = MPI_Aint_diff(displacements[4], base_address);
  MPI_Datatype types[5] = {MPI_INT, MPI_INT, MPI_INT, MPI_INT, MPI_DOUBLE};
  MPI_Type_create_struct(5, lengths, displacements, types, &SubsetVector);
  MPI_Type_commit(&SubsetVector);*/

  // Maybe use this for defining a custom type --> set of row vectors for sub-matrix
  // https://stackoverflow.com/questions/41493350/c-mpi-parallel-processing-of-column-arrays

  // Check if number of processes assumption true
  if (num_procs == 0) {
    fprintf(stderr, "%d: Starting Rabenseifner numprocs=%d\n", rank, num_procs);
    return;
  }
  int n_rounds = 0;
  int n_processors = num_procs;
  while (n_processors >>= 1) ++n_rounds;

  // TODO: Extend Rabenseifner with an extension for non-power-of-2 processes

  if (num_procs != (1 << n_rounds)) {
    fprintf(stderr, "%d: Starting Rabenseifner with non-power-of-2 numprocs=%d\n", rank, num_procs);
    return;
  }

  int round, recv_rank;
  int one = 1UL;
  int matrix_size = N * M;
  int n_rows = N;
  int n_cols = M;
  MPI_Status status;
  MPI_Request request;

  // Directly use result buffer to store intermediate result
  double* recvMatrixPtr = new double[matrix_size];
  double* sendMatrixPtr = new double[matrix_size];
  // Store all vectors in these buffers
  std::vector<SubVec> vectorBuffer_A;
  std::vector<SubVec> vectorBuffer_B;
  // Add my vectors to the vector buffers
  vector vec_A(N);
  for (int i = 0; i < N; i++) {
    vec_A[i] = a[i];
  }
  vector vec_B(M);
  for (int i = 0; i < M; i++) {
    vec_B[i] = b[i];
  }
  vectorBuffer_A.push_back(SubVec{N, 0, N, 1, rank, &vec_A});
  vectorBuffer_B.push_back(SubVec{M, 0, M, 0, rank, &vec_B});

  // [RABENSEIFNER INDICES]
  // Number of splits in the final matrix depend on the number of processes
  int n_vertical_partitions, n_horizontal_partitions;
  if (num_procs == 1) {
    n_vertical_partitions = 1;
    n_horizontal_partitions = 1;
  } else if (n_rounds % 2 == 0) {
    // even #rounds
    int n_splits = (int)sqrt(num_procs);
    n_vertical_partitions = n_splits;
    n_horizontal_partitions = n_splits;
  } else {
    // uneven #rounds
    n_vertical_partitions = (int)sqrt((1 << (n_rounds + 1)));
    n_horizontal_partitions = (int)sqrt((1 << (n_rounds - 1)));
  }
  // Prepare Indices List for Vertical and Horizontal Splitting
  std::vector<int> all_indices_vertical(n_vertical_partitions + 1, 0);
  std::vector<int> all_indices_horizontal(n_horizontal_partitions + 1, 0);
  all_indices_vertical[n_vertical_partitions] = n_rows;
  all_indices_horizontal[n_horizontal_partitions] = n_cols;
  int chunk_size_vertical = n_rows / n_vertical_partitions;
  int chunk_size_horizontal = n_cols / n_horizontal_partitions;
  for (int i = 1; i < n_vertical_partitions; i++) {
    all_indices_vertical[i] = all_indices_vertical[i - 1] + chunk_size_vertical;
  }
  for (int i = 1; i < n_horizontal_partitions; i++) {
    all_indices_horizontal[i] = all_indices_horizontal[i - 1] + chunk_size_horizontal;
  }
  // My Indices for individual rounds
  // --> symmetric: used once forward (vector distribution), once backward (submatrix-distribution)
  std::vector<int> my_send_indices_lower_A(n_rounds, 0);
  std::vector<int> my_send_indices_upper_A(n_rounds, 0);
  std::vector<int> my_recv_indices_lower_A(n_rounds, 0);
  std::vector<int> my_recv_indices_upper_A(n_rounds, 0);
  std::vector<int> my_send_indices_lower_B(n_rounds, 0);
  std::vector<int> my_send_indices_upper_B(n_rounds, 0);
  std::vector<int> my_recv_indices_lower_B(n_rounds, 0);
  std::vector<int> my_recv_indices_upper_B(n_rounds, 0);
  int current_idx_lower_A = 0;
  int current_idx_lower_B = 0;
  int current_idx_upper_A = n_vertical_partitions;
  int current_idx_upper_B = n_horizontal_partitions;
  int residual_splits_A = n_vertical_partitions;
  int residual_splits_B = n_horizontal_partitions;
  int middle_idx;
  for (round = n_rounds - 1; round >= 0; round--) {
    // Favor splitting vertically
    if (residual_splits_A <= residual_splits_B) {
      // --> split vertically if there are more splits vertically left
      residual_splits_A /= 2;
      middle_idx = ((current_idx_lower_A + current_idx_upper_A) / 2);
      if (rank < middle_idx) {
        my_recv_indices_lower_A[round] = current_idx_lower_A;
        my_recv_indices_upper_A[round] = middle_idx;
        my_send_indices_lower_A[round] = middle_idx;
        my_send_indices_upper_A[round] = current_idx_upper_A;

        current_idx_upper_A = middle_idx;
      } else {
        my_send_indices_lower_A[round] = current_idx_lower_A;
        my_send_indices_upper_A[round] = middle_idx;
        my_recv_indices_lower_A[round] = middle_idx;
        my_recv_indices_upper_A[round] = current_idx_upper_A;

        current_idx_lower_A = middle_idx;
      }
    } else {
      // --> split horizontally if there are more horizontal spltis left
      residual_splits_B /= 2;
      middle_idx = ((current_idx_lower_B + current_idx_upper_B) / 2);
      if (rank < middle_idx) {
        my_recv_indices_lower_B[round] = current_idx_lower_B;
        my_recv_indices_upper_B[round] = middle_idx;
        my_send_indices_lower_B[round] = middle_idx;
        my_send_indices_upper_B[round] = current_idx_upper_B;

        current_idx_upper_B = middle_idx;
      } else {
        my_send_indices_lower_B[round] = current_idx_lower_B;
        my_send_indices_upper_B[round] = middle_idx;
        my_recv_indices_lower_B[round] = middle_idx;
        my_recv_indices_upper_B[round] = current_idx_upper_B;

        current_idx_lower_B = middle_idx;
      }
    }
  }
  fprintf(stderr, "%d: n_vertical_partitions: %d, n_horizontal_partitions: %d\n", rank,
      n_vertical_partitions, n_horizontal_partitions);

  fprintf(stderr, "%d: Receive Lower Indices A: %s, upper indices A: %s\n", rank,
      to_string(my_recv_indices_lower_A).c_str(), to_string(my_recv_indices_upper_A).c_str());
  fprintf(stderr, "%d: Receive Lower Indices B: %s, upper indices B: %s\n", rank,
      to_string(my_recv_indices_lower_B).c_str(), to_string(my_recv_indices_upper_B).c_str());

  fprintf(stderr, "%d: SEND Lower Indices A: %s, upper indices A: %s\n", rank,
      to_string(my_send_indices_lower_A).c_str(), to_string(my_send_indices_upper_A).c_str());
  fprintf(stderr, "%d: SEND Lower Indices B: %s, upper indices B: %s\n", rank,
      to_string(my_send_indices_lower_B).c_str(), to_string(my_send_indices_upper_B).c_str());

  // Initialize a buffer for sending and receiving
  int transfer_size_A = (5 + chunk_size_vertical) * ((num_procs + 1) / 2);
  int transfer_size_B = (5 + chunk_size_horizontal) * ((num_procs + 1) / 2);
  int max_transfer_size = transfer_size_A;
  if (transfer_size_B > max_transfer_size) {
    max_transfer_size = transfer_size_B;
  }
  double* send_buffer = new double[max_transfer_size];
  double* recv_buffer = new double[max_transfer_size];

  fprintf(stderr, "%d: Starting butterfly reduce scatter\n", rank);

  // [RABENSEIFNER - BUTTERFLY REDUCE SCATTER]
  // Distribute Vectors using a butterfly-scatter
  int idx_lower_send_A, idx_lower_send_B, idx_upper_send_A, idx_upper_send_B;
  int chunk_size_send, chunk_size_recv;
  for (round = n_rounds - 1; round >= 0; round--) {
    // receiver rank (from who we should expect data), is the same rank we send data to
    int bit_vec = (one << round);
    recv_rank = rank ^ bit_vec;

    idx_lower_send_A = all_indices_vertical[my_send_indices_lower_A[round]];
    idx_upper_send_A = all_indices_vertical[my_send_indices_upper_A[round]];
    idx_lower_send_B = all_indices_horizontal[my_send_indices_lower_B[round]];
    idx_upper_send_B = all_indices_horizontal[my_send_indices_upper_B[round]];
    chunk_size_recv = max_transfer_size;

    // set the send buffer and send
    setSendVector(&vectorBuffer_A, &vectorBuffer_B, send_buffer, &chunk_size_send, idx_lower_send_A, idx_lower_send_B,
        idx_upper_send_A, idx_upper_send_B);
    mpi_timer(MPI_Isend, send_buffer, chunk_size_send, MPI_DOUBLE, recv_rank, TAG_RABENSEIFNER_SCATTER, comm, &request);

    // receive elements
    mpi_timer(MPI_Recv, recv_buffer, chunk_size_recv, MPI_DOUBLE, recv_rank, TAG_RABENSEIFNER_SCATTER, comm, &status);
    MPI_Get_count(&status, MPI_DOUBLE, &chunk_size_recv);
    addReceivedVectors(&vectorBuffer_A, &vectorBuffer_B, recv_buffer, chunk_size_recv);

    MPI_Wait(&request, &status);
  }
  delete[] send_buffer;
  delete[] recv_buffer;
  // sort the vector buffers in increasing order
  std::sort(std::begin(vectorBuffer_A), std::end(vectorBuffer_B),
      [](SubVec x, SubVec y) { return x.owner_rank > y.owner_rank; });

  fprintf(stderr, "%d: Computing local submatrix\n", rank);

  // [COMPUTE OWN SUBMATRIX]
  if (vectorBuffer_A.size() != vectorBuffer_B.size()) {
    fprintf(stderr, "%d: UNEQUAL VECTOR BUFFER SIZES", rank);
    throw std::runtime_error(
        "BUG Rabenseifner-Scatter unequal vector buffer sizes for process " + std::to_string(rank));
  }
  int compute_n_rows = all_indices_vertical[current_idx_upper_A] - all_indices_vertical[current_idx_lower_A];
  int compute_n_cols = all_indices_vertical[current_idx_upper_B] - all_indices_vertical[current_idx_lower_B];
  vector computeA(compute_n_rows, 0);
  vector computeB(compute_n_cols, 0);
  for (int i = 0; i < num_procs; i++) {
    fprintf(stderr, "%d: Computing local submatrix - %d - 1\n", rank, i);
    int start_idx_A = current_idx_lower_A - vectorBuffer_A[i].start_idx;
    fprintf(stderr, "%d: Computing local submatrix - %d - 2\n", rank, i);
    int start_idx_B = current_idx_lower_B - vectorBuffer_B[i].start_idx;
    fprintf(stderr,
        "%d: Computing local submatrix - %d - 3, start_idx_A=%d, start_idx_B=%d, current_idx_lower_A=%d, "
        "current_idx_upper_A=%d "
        "current_idx_lower_B=%d, current_idx_upper_B=%d, compute_n_rows=%d, compute_n_cols=%d\n",
        rank, i, start_idx_A, start_idx_B, current_idx_lower_A, current_idx_upper_A, current_idx_lower_B,
        current_idx_upper_B, compute_n_rows, compute_n_cols);
    // copy the right elements into the vectors
    double* vec_A_ptr = (vectorBuffer_A[i].vecPtr)->data();
    for (int j = 0; j < compute_n_rows; j++) {
      computeA[j] = vec_A_ptr[start_idx_A + j];
    }
    fprintf(stderr, "%d: Computing local submatrix - %d - 4\n", rank, i);
    double* vec_B_ptr = (vectorBuffer_B[i].vecPtr)->data();
    for (int j = 0; j < compute_n_cols; j++) {
      computeB[j] = vec_B_ptr[start_idx_B + j];
    }
    fprintf(stderr, "%d: Computing local submatrix - %d - 5\n", rank, i);
    // add outer product to submatrix
    add_submatrix_outer_product(result, current_idx_lower_A, current_idx_lower_B, computeA, computeB);
  }

  fprintf(stderr, "%d: Butterfly Allgather\n", rank);

  // [RABENSEIFNER - BUTTERFLY ALLGATHER]
  // Distribute sub-matrices among all processes using a butterfly.
  int idx_lower_send_row, idx_upper_send_row, idx_lower_send_col, idx_upper_send_col;
  int idx_lower_recv_row, idx_upper_recv_row, idx_lower_recv_col, idx_upper_recv_col;
  for (round = 0; round < n_rounds; round++) {
    int bit_vec = (one << round);
    recv_rank = rank ^ bit_vec;

    idx_lower_send_row = all_indices_vertical[my_send_indices_lower_A[round]];
    idx_upper_send_row = all_indices_vertical[my_send_indices_upper_A[round]];
    idx_lower_send_col = all_indices_horizontal[my_send_indices_lower_B[round]];
    idx_upper_send_col = all_indices_horizontal[my_send_indices_upper_B[round]];
    chunk_size_send = (idx_upper_send_row - idx_lower_send_row) * (idx_upper_send_col - idx_lower_send_col);
    idx_lower_recv_row = all_indices_vertical[my_recv_indices_lower_A[round]];
    idx_upper_recv_row = all_indices_vertical[my_recv_indices_upper_A[round]];
    idx_lower_recv_col = all_indices_horizontal[my_recv_indices_lower_B[round]];
    idx_upper_recv_col = all_indices_horizontal[my_recv_indices_upper_B[round]];
    chunk_size_recv = (idx_upper_recv_row - idx_lower_recv_row) * (idx_upper_recv_col - idx_lower_recv_col);

    // send submatrix
    submatrixToBuffer(
        result, sendMatrixPtr, idx_lower_send_row, idx_upper_send_row, idx_lower_send_col, idx_upper_send_col);
    mpi_timer(MPI_Ssend, sendMatrixPtr, chunk_size_send, MPI_DOUBLE, recv_rank, TAG_RABENSEIFNER_SCATTER, comm);
    // receive submatrix
    mpi_timer(MPI_Recv, recvMatrixPtr, chunk_size_recv, MPI_DOUBLE, recv_rank, TAG_RABENSEIFNER_SCATTER, comm, &status);
    copyBufferToSubmatrix(
        result, recvMatrixPtr, idx_lower_recv_row, idx_upper_recv_row, idx_lower_recv_col, idx_upper_recv_col);
  }
  delete[] sendMatrixPtr;
  delete[] recvMatrixPtr;
}

void addReceivedVectors(
    std::vector<SubVec>* buffer_A, std::vector<SubVec>* buffer_B, double* receiveBuffer, int n_recv_elements) {
  int idx = 0, vec_idx;
  int n_elements_in_vec, vec_start_idx, vec_end_idx, vec_is_A, vec_owner_rank;
  while (idx < n_recv_elements) {
    n_elements_in_vec = (int)receiveBuffer[idx];
    vec_start_idx = (int)receiveBuffer[idx + 1];
    vec_end_idx = (int)receiveBuffer[idx + 2];
    vec_is_A = (int)receiveBuffer[idx + 3];
    vec_owner_rank = (int)receiveBuffer[idx + 4];
    idx += 5;
    vector vec(n_elements_in_vec, 0);
    for (vec_idx = 0; vec_idx < n_elements_in_vec; vec_idx++) {
      vec[vec_idx] = receiveBuffer[idx];
      idx++;
    }
    if (vec_is_A) {
      buffer_A->push_back(SubVec{n_elements_in_vec, vec_start_idx, vec_end_idx, vec_is_A, vec_owner_rank, &vec});
    } else {
      buffer_B->push_back(SubVec{n_elements_in_vec, vec_start_idx, vec_end_idx, vec_is_A, vec_owner_rank, &vec});
    }
  }
  return;
}
void setSendVector(std::vector<SubVec>* buffer_A, std::vector<SubVec>* buffer_B, double* sendBuffer,
    int* n_send_elements, int lower_index_A, int lower_index_B, int upper_index_A, int upper_index_B) {
  double write_n_elements_int = (upper_index_A - lower_index_A);
  double write_n_elements = (double)(upper_index_A - lower_index_A);
  double write_lower_idx = (double)lower_index_A;
  double write_upper_idx = (double)upper_index_A;
  double write_is_A = (double)1;
  double write_is_B = (double)0;

  int buffer_idx = 0, vec_idx;
  // write out contents of buffer-A
  for (std::vector<SubVec>::size_type i = 0; i != buffer_A->size(); i++) {
    if ((*buffer_A)[i].start_idx <= lower_index_A && (*buffer_A)[i].end_idx >= upper_index_A) {
      sendBuffer[buffer_idx] = write_n_elements;
      sendBuffer[buffer_idx + 1] = write_lower_idx;
      sendBuffer[buffer_idx + 2] = write_upper_idx;
      sendBuffer[buffer_idx + 3] = write_is_A;
      sendBuffer[buffer_idx + 4] = (double)(*buffer_A)[i].owner_rank;
      buffer_idx += 5;

      vec_idx = lower_index_A - (*buffer_A)[i].start_idx;
      for (int j = 0; j < write_n_elements_int; j++) {
        sendBuffer[buffer_idx] = (*(*buffer_A)[i].vecPtr)[vec_idx + j];
        buffer_idx++;
      }
    }
  }
  // write out contents of buffer-B
  write_n_elements_int = (upper_index_B - lower_index_B);
  write_n_elements = (double)(upper_index_B - lower_index_B);
  write_lower_idx = (double)lower_index_B;
  write_upper_idx = (double)upper_index_B;
  for (std::vector<SubVec>::size_type i = 0; i != buffer_B->size(); i++) {
    if ((*buffer_B)[i].start_idx <= lower_index_B && (*buffer_B)[i].end_idx >= upper_index_B) {
      sendBuffer[buffer_idx] = write_n_elements;
      sendBuffer[buffer_idx + 1] = write_lower_idx;
      sendBuffer[buffer_idx + 2] = write_upper_idx;
      sendBuffer[buffer_idx + 3] = write_is_B;
      sendBuffer[buffer_idx + 4] = (double)(*buffer_B)[i].owner_rank;
      buffer_idx += 5;

      vec_idx = lower_index_B - (*buffer_B)[i].start_idx;
      for (int j = 0; j < write_n_elements_int; j++) {
        sendBuffer[buffer_idx] = (*(*buffer_B)[i].vecPtr)[vec_idx + j];
        buffer_idx++;
      }
    }
  }
  *n_send_elements = buffer_idx;
  return;
}
void submatrixToBuffer(matrix& M, double* sendbuffer, int start_row, int end_row, int start_col, int end_col) {
  int idx = 0;
  for (int i = start_row; i < end_row; i++) {
    for (int j = start_col; j < end_col; j++) {
      sendbuffer[idx] = M.get(i, j);
      idx++;
    }
  }
  return;
}
void copyBufferToSubmatrix(matrix& M, double* sendbuffer, int start_row, int end_row, int start_col, int end_col) {
  int idx = 0;
  for (int i = start_row; i < end_row; i++) {
    for (int j = start_col; j < end_col; j++) {
      M.get(i, j) = sendbuffer[idx];
      idx++;
    }
  }
  return;
}

} // namespace impls::rabenseifner_scatter
