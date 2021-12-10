#include "allgather_async/impl.hpp"

#include <cassert>

namespace impls::allgather_async {

/**
 * Performs allgather but with async requests.
 *
 * Whenever a vector from a process is received the outer-product can be computed while the other vectors are still
 * being received.
 */
void allgather_async::compute(const std::vector<vector>& a_in, const std::vector<vector>& b_in, matrix& result) {
  const auto& a = a_in[rank];
  const auto& b = b_in[rank];

  vector send_vec = a;
  send_vec.insert(send_vec.end(), b.begin(), b.end());

  int vec_size = send_vec.size();

  std::vector<MPI_Request> send_reqs;
  std::vector<MPI_Request> recv_reqs;
  std::vector<vector> rec(num_procs, vector(vec_size));
  // Maps a request index to the process rank
  std::vector<int> rec_map;

  /*
   * Asynchronously send and receive vectors to/from all other processes.
   */
  for (int i = 0; i < num_procs; i++) {
    if (i == rank) {
      continue;
    }

    send_reqs.push_back(MPI_REQUEST_NULL);
    mpi_timer(MPI_Isend, send_vec.data(), vec_size, MPI_DOUBLE, i, TAG_ALLGATHER_ASYNC, comm, &send_reqs.back());

    recv_reqs.push_back(MPI_REQUEST_NULL);
    rec_map.push_back(i);
    mpi_timer(MPI_Irecv, rec[i].data(), vec_size, MPI_DOUBLE, i, TAG_ALLGATHER_ASYNC, comm, &recv_reqs.back());
  }

  // Calculate own outer product
  result.set_outer_product(a, b);

  /*
   * Iterative wait for requests to finish. For each finished receive request, we can calculate one outer product and
   * add it to the matrix.
   */
  while (true) {
    int idx;
    mpi_timer(MPI_Waitany, recv_reqs.size(), recv_reqs.data(), &idx, MPI_STATUS_IGNORE);

    // No active requests left
    if (idx == MPI_UNDEFINED) {
      break;
    }

    const auto& vec = rec[rec_map[idx]];

    result.add_outer_product(N, vec.data(), M, &vec[N]);
  }

  mpi_timer(MPI_Waitall, send_reqs.size(), send_reqs.data(), MPI_STATUSES_IGNORE);
}

} // namespace impls::allgather_async
