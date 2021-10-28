#ifndef CODE_DSOP_H
#define CODE_DSOP_H

#include <mpi.h>

#include "common.h"
#include "matrix.h"

// dsop is the interface that dsop implementations should satisfy
class dsop {
 protected:
  const MPI_Comm comm;
  const int rank;
  const int num_procs;
  const int N;
  const int M;

 public:
  dsop(MPI_Comm comm, int rank, int num_procs, int N, int M) : comm(comm), rank(rank), num_procs(num_procs), N(N), M(M) {}
  virtual ~dsop() = default;

  virtual matrix compute(const std::vector<vector>& a_in, const std::vector<vector>& b_in) = 0;
};

#endif // CODE_DSOP_H
