#include "bruck_async/impl.hpp"

#include <cassert>

namespace impls::bruck_async {
/**
 * The number of rounds is ceil(log2(num_procs))
 */
static int get_num_rounds(uint32_t num_procs) {
  int num_leading_zeroes = __builtin_clz(num_procs);
  int result = 32 - num_leading_zeroes - 1;

  if ((int)num_procs > (1 << result)) {
    result++;
  }

  return result;
}

/**
 * Performs the allgather algorithm by Bruck but with nonblocking sends and computing outer products while waiting for
 * more data.
 *
 * In each round, while waiting for the data, we process the data received in the previous round.
 */
void bruck_async::compute(const std::vector<vector>& a_in, const std::vector<vector>& b_in, matrix& result) {
  const auto& a = a_in[rank];
  const auto& b = b_in[rank];

  int vec_size = a.size() + b.size();

  std::vector<double> rbuf(num_procs * vec_size);
  std::copy(a.begin(), a.end(), rbuf.begin());
  std::copy(b.begin(), b.end(), rbuf.begin() + a.size());

  int num_rounds = get_num_rounds(num_procs);

  /**
   * How many vector pairs we already have in our buffer
   *
   * This means rbuf[0, chunks_received * vec_size) is valid
   */
  int chunks_received = 1;

  /*
   * Range (end is exclusive) of vec_size chunks that we still need to compute with.
   */
  int to_compute_start = 0;
  int to_compute_end = 1;

  for (int i = 0; i < num_rounds; i++) {
    int offset = 1 << i;
    int target = ((rank - offset) + num_procs) % num_procs;
    int source = (rank + offset) % num_procs;

    MPI_Request send_req, recv_req;

    /*
     * In the last round for non-power-of-2 nodes, the number of remaining chunks will not match the exponential
     * progression.
     */
    int chunks_remaining = num_procs - chunks_received;
    int num_chunks_send = std::min(offset, chunks_remaining);
    int num_send = vec_size * num_chunks_send;

    auto* base_send = rbuf.data();
    auto* base_recv = rbuf.data() + chunks_received * vec_size;

    mpi_timer(MPI_Isend, base_send, num_send, MPI_DOUBLE, target, TAG_BRUCK_ASYNC, comm, &send_req);
    mpi_timer(MPI_Irecv, base_recv, num_send, MPI_DOUBLE, source, TAG_BRUCK_ASYNC, comm, &recv_req);

    for (int z = to_compute_start; z < to_compute_end; z++) {
      int rbuf_offset = z * vec_size;
      for (int x = 0; x < N; x++) {
        for (int y = 0; y < M; y++) {
          result.get(x, y) += rbuf[rbuf_offset + x] * rbuf[rbuf_offset + N + y];
        }
      }
    }

    to_compute_start = to_compute_end;
    to_compute_end += num_chunks_send;
    chunks_received += num_chunks_send;

    mpi_timer(MPI_Wait, &send_req, MPI_STATUS_IGNORE);
    mpi_timer(MPI_Wait, &recv_req, MPI_STATUS_IGNORE);
  }

  for (int z = to_compute_start; z < to_compute_end; z++) {
    int rbuf_offset = z * vec_size;
    for (int x = 0; x < N; x++) {
      for (int y = 0; y < M; y++) {
        result.get(x, y) += rbuf[rbuf_offset + x] * rbuf[rbuf_offset + N + y];
      }
    }
  }
}
} // namespace impls::bruck_async
