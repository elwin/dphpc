#include "allreduce_butterfly_segmented/impl.hpp"

// Number of bytes in a segment
static constexpr int SEG_SIZE = 1 << 17;
// Number of elements in a segment
static constexpr int SEG_EL = SEG_SIZE / sizeof(double);

static_assert(SEG_SIZE % sizeof(double) == 0);

namespace impls::allreduce_butterfly_segmented {

/**
 * Performs allreduce with explicit butterfly communication.
 *
 * The buttefly communication is done pipelined. While adding received matrices to the result, already computed chunks
 * are already sent for the next round.
 */
void allreduce_butterfly_segmented::compute(
    const std::vector<vector>& a_in, const std::vector<vector>& b_in, matrix& result) {
  const auto& a = a_in[rank];
  const auto& b = b_in[rank];

  int matrix_size = N * M;
  // num_segments = ceil(N * M / SEG_EL)
  const int num_segments = (matrix_size + SEG_EL - 1) / SEG_EL;
  int last_segment_elements = matrix_size % SEG_EL;

  if (last_segment_elements == 0) {
    last_segment_elements = SEG_EL;
  }

  assert((num_segments - 1) * SEG_EL + last_segment_elements == matrix_size);

  // Write initial outer product to result matrix
  result.set_outer_product(a, b);

  if (num_procs == 1) {
    return;
  }

  auto receivedMatrix = std::make_unique<double[]>(matrix_size);
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
  bool i_am_idle_rank = non_power_of_2_rounds && (rank >= power_2_ranks); // whether I am idle or not
  bool i_am_idle_partner =
      non_power_of_2_rounds && (rank < (num_procs - power_2_ranks)); // whether I am partner of an idle process
  int idle_partner_rank = 0;

  if (i_am_idle_partner) {
    idle_partner_rank = rank + power_2_ranks;
  } else if (i_am_idle_rank) {
    idle_partner_rank = rank - power_2_ranks;
  }

  // They should be mutually exclusive
  assert(!(i_am_idle_rank && i_am_idle_partner));

  // Start Reducing to nearest power of 2
  if (i_am_idle_rank) {
    // send
    mpi_timer(MPI_Send, resultPtr, matrix_size, MPI_DOUBLE, idle_partner_rank, TAG_ALLREDUCE_BUTTERFLY_SEGMENTED_REDUCE,
        comm);
  } else if (i_am_idle_partner) {
    // receive
    mpi_timer(MPI_Recv, receivedMatrix.get(), matrix_size, MPI_DOUBLE, idle_partner_rank,
        TAG_ALLREDUCE_BUTTERFLY_SEGMENTED_REDUCE, comm, MPI_STATUS_IGNORE);
    // add received data to the  temporary matrix
    for (int i = 0; i < matrix_size; i++) {
      resultPtr[i] += receivedMatrix[i];
    }
  }

  // [START BUTTERFLY ROUNDS]

  if (!i_am_idle_rank) {
    int recv_rank = rank ^ 1UL;
    mpi_timer(MPI_Sendrecv, resultPtr, matrix_size, MPI_DOUBLE, recv_rank, TAG_ALLREDUCE_BUTTERFLY_SEGMENTED,
        receivedMatrix.get(), matrix_size, MPI_DOUBLE, recv_rank, TAG_ALLREDUCE_BUTTERFLY_SEGMENTED, comm, MPI_STATUS_IGNORE);

    for (int round = 1; round < n_rounds; round++) {
      // receiver rank (from who we should expect data), is the same rank we send data to
      int recv_rank = rank ^ (1UL << round);

      // add received data to the result

      MPI_Request send_req = MPI_REQUEST_NULL;
      for (int seg = 0; seg < num_segments; seg++) {
        bool is_last = seg == num_segments - 1;

        int base = seg * SEG_EL;

        // Number of elements in currently processed segment
        const int segment_elements = is_last ? last_segment_elements : SEG_EL;

        for (int i = base; i < base + segment_elements; i++) {
          resultPtr[i] += receivedMatrix[i];
        }

        if (seg > 0) {
          mpi_timer(MPI_Recv, &receivedMatrix[base - SEG_EL], SEG_EL, MPI_DOUBLE, recv_rank,
              TAG_ALLREDUCE_BUTTERFLY_SEGMENTED, comm, MPI_STATUS_IGNORE);
        }

        mpi_timer(MPI_Wait, &send_req, MPI_STATUS_IGNORE);
        mpi_timer(MPI_Isend, resultPtr + base, segment_elements, MPI_DOUBLE, recv_rank,
            TAG_ALLREDUCE_BUTTERFLY_SEGMENTED, comm, &send_req);
      }

      mpi_timer(MPI_Recv, &receivedMatrix[matrix_size - last_segment_elements], last_segment_elements, MPI_DOUBLE,
          recv_rank, TAG_ALLREDUCE_BUTTERFLY_SEGMENTED, comm, MPI_STATUS_IGNORE);
      mpi_timer(MPI_Wait, &send_req, MPI_STATUS_IGNORE);
    }

    for (int i = 0; i < matrix_size; i++) {
      resultPtr[i] += receivedMatrix[i];
    }
  }
  // [END BUTTERFLY ROUNDS]

  // [FINISH Reducing to nearest power of 2]
  if (i_am_idle_partner) {
    // send
    mpi_timer(MPI_Send, resultPtr, matrix_size, MPI_DOUBLE, idle_partner_rank, TAG_ALLREDUCE_BUTTERFLY_SEGMENTED_REDUCE,
        comm);
  } else if (i_am_idle_rank) {
    // receive --> puts results automatically in result
    mpi_timer(MPI_Recv, resultPtr, matrix_size, MPI_DOUBLE, idle_partner_rank, TAG_ALLREDUCE_BUTTERFLY_SEGMENTED_REDUCE,
        comm, MPI_STATUS_IGNORE);
  }
}

} // namespace impls::allreduce_butterfly_segmented
