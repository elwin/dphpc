#pragma once

#include <mpi.h>

#include <memory>

namespace impls::allreduce {
std::unique_ptr<double[]> run(MPI_Comm comm, std::unique_ptr<double[]> A, int N, std::unique_ptr<double[]> B, int M, int rank, int numprocs);
}
