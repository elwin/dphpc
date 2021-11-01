#ifndef CODE_DSOP_H
#define CODE_DSOP_H

#include <mpi.h>

#include "common.h"
#include "matrix.h"
#include "util.hpp"

// dsop is the interface that dsop implementations should satisfy
class dsop {
 protected:
  const MPI_Comm comm;
  const int rank;
  const int num_procs;
  const int N;
  const int M;

  int64_t mpi_time{0};

 public:
  dsop(MPI_Comm comm, int rank, int num_procs, int N, int M)
      : comm(comm), rank(rank), num_procs(num_procs), N(N), M(M) {}
  virtual ~dsop() = default;

  virtual void compute(const std::vector<vector>& a_in, const std::vector<vector>& b_in, matrix& result) = 0;

  template <typename F, typename... Args>
  auto mpi_timer(F& f, Args&&... args) {
    timer t;
    const auto& r = f(std::forward<Args>(args)...);
    mpi_time += t.finish();

    return r;
  }

  int64_t get_mpi_time() const {
    return mpi_time;
  }
};

#endif // CODE_DSOP_H
