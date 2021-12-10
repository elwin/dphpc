#include "grabenseifner_allgather_segmented/impl.hpp"

#include <math.h>

#include <cmath>

#include "grabenseifner_allgather/impl.hpp"

// Number of bytes in a segment
static constexpr int SEG_SIZE = 4096;
// Number of elements in a segment
static constexpr int SEG_EL = SEG_SIZE / sizeof(double);

static_assert(SEG_SIZE % sizeof(double) == 0);

namespace impls::grabenseifner_allgather_segmented {

/**
 * Generalized Rabenseifner using two allgather rounds with pipelining
 */
void grabenseifner_allgather_segmented::compute(
    const std::vector<vector>& a_in, const std::vector<vector>& b_in, matrix& result) {
  const auto& a = a_in[rank];
  const auto& b = b_in[rank];

  int n_rows = (int)a.size();
  int n_cols = (int)b.size();

  if (n_rows < num_procs) {
    throw std::runtime_error("grabenseifner_allgather_segmented: Fewer rows than processes");
  }

  /*
   * If the sent vector size is smaller than the number of elements in a segment, we can just as well use the
   * non-segmented implementation
   */
  if (b.size() < SEG_EL) {
    grabenseifner_allgather::grabenseifner_allgather impl(comm, rank, num_procs, N, M);
    impl.compute(a_in, b_in, result);
    return;
  }

  int my_rank = (int)rank;
  // partition the rows of the final matrix among all processes
  int my_n_rows = n_rows / num_procs;
  // Number of rows the last process is responsible for
  const int last_n_rows = n_rows - ((num_procs - 1) * my_n_rows);
  /*
   * Number of additional rows, the last process has in relation to other processes.
   */
  const int residual_rows = last_n_rows - my_n_rows;
  const int last_chunk_start_row = num_procs * my_n_rows;

  /*
   * Each process is responsible for rows [my_start_row, my_start_row + my_n_rows)
   */
  int my_start_row = my_n_rows * rank;

  /*
   * Number of doubles of the final matrix this process sends in the last
   * allgather round
   */
  const int my_block_size = my_n_rows * n_cols;

  bool special_last_block = (my_n_rows != last_n_rows);
  if (my_rank == num_procs - 1) {
    my_n_rows = last_n_rows;
  }

  // num_segments = ceil(M / SEG_EL)
  const int num_segments = (b.size() + SEG_EL - 1) / SEG_EL;

  vector all_as(n_rows * num_procs);
  /*
   * Collects all values in the B vectors for all processes.
   *
   * The B vector for a process is not stored contiguously, but because of the segmentation, only SEG_EL elements are
   * contiguous.
   *
   * The B vector for process proc_i in segment seg_i starts at index:
   *
   * seg_i * SEG_EL * num_procs + proc_i * SEG_EL = SEG_EL * (seg_i * num_procs + proc_i)
   */
  vector all_bs(n_cols * num_procs);

  // allgather round

  // First receive the A vectors
  // TODO use blocking allgather with the smaller vector
  mpi_timer(MPI_Allgather, a.data(), a.size(), MPI_DOUBLE, all_as.data(), a.size(), MPI_DOUBLE, comm);

  MPI_Request req = MPI_REQUEST_NULL;
  mpi_timer(MPI_Iallgather, b.data(), SEG_EL, MPI_DOUBLE, all_bs.data(), SEG_EL, MPI_DOUBLE, comm, &req);

  const int segment_size = SEG_EL * num_procs;
  // Number of elements (per process) in last segment
  int last_segment_elements = (b.size() % SEG_EL);

  if (last_segment_elements == 0) {
    last_segment_elements = SEG_EL;
  }

  for (int seg_i = 0; seg_i < num_segments; seg_i++) {
    bool is_last = seg_i == num_segments - 1;
    // Index in all_bs where this segment starts
    int base_index = seg_i * segment_size;
    int base_column = seg_i * SEG_EL;

    // Wait for previous allgather to complete
    mpi_timer(MPI_Wait, &req, MPI_STATUS_IGNORE);

    // Allgather next segment
    if (!is_last) {
      const int segment_elements = seg_i == num_segments - 2 ? last_segment_elements : SEG_EL;
      mpi_timer(MPI_Iallgather, b.data() + base_column + SEG_EL, segment_elements, MPI_DOUBLE,
          all_bs.data() + base_index + segment_size, segment_elements, MPI_DOUBLE, comm, &req);
    }

    // Number of elements in currently processed segment
    const int segment_elements = is_last ? last_segment_elements : SEG_EL;

    // Compute from current segment while next allgather is in flight
    const int row_idx_upper = my_start_row + my_n_rows;
    for (int proc_i = 0; proc_i < num_procs; proc_i++) {
      for (int row_i = my_start_row; row_i < row_idx_upper; row_i++) {
        double a_val = all_as[proc_i * n_rows + row_i];
        for (int col_i = 0; col_i < segment_elements; col_i++) {
          double b_val = all_bs[base_index + proc_i * segment_elements + col_i];
          result.get(row_i, base_column + col_i) += a_val * b_val;
        }
      }
    }
  }

  if (special_last_block) {
    // second allgather round matrix prefix
    mpi_timer(MPI_Allgather, &result.get(my_start_row, 0), my_block_size, MPI_DOUBLE, result.get_ptr(), my_block_size,
        MPI_DOUBLE, comm);
    // last process broadcasts residual chunk to all processes --> last processor is the root of this broadcast
    mpi_timer(
        MPI_Bcast, &result.get(last_chunk_start_row, 0), (residual_rows * n_cols), MPI_DOUBLE, (num_procs - 1), comm);
  } else {
    // second allgather round
    mpi_timer(MPI_Allgather, &result.get(my_start_row, 0), my_block_size, MPI_DOUBLE, result.get_ptr(), my_block_size,
        MPI_DOUBLE, comm);
  }
}
} // namespace impls::grabenseifner_allgather_segmented
