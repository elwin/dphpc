#ifndef CODE_DSOP_H
#define CODE_DSOP_H

#include "common.h"
#include "matrix.h"
#include "mpi.h"

// dsop is the interface that dsop implementations should satisfy
class dsop {
 protected:
  MPI_Comm comm;
  int rank;
  int num_procs;

 public:
  dsop(MPI_Comm comm, int rank, int num_procs) : comm(comm), rank(rank), num_procs(num_procs) {}
  dsop() {}

  virtual void load(std::vector<vector>* a, std::vector<vector>* b) = 0;

  virtual matrix* compute() = 0;
};

#endif // CODE_DSOP_H
