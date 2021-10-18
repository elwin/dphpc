#include <mpi.h>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <iterator>
#include <memory>
#include <vector>

#include "util.hpp"

#define RANGE_START -1.0
#define RANGE_END 1.0

static constexpr int N = 3;
static constexpr int M = 3;

int main(int argc, char* argv[]) {
  int rank;
  int numprocs;

  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  bool is_root = rank == 0;

  printf("Starting rank=%d size=%d\n", rank, numprocs);
  // Use the rank as the seed
  auto A = get_random((uint64_t)rank, N, RANGE_START, RANGE_END);
  auto B = get_random((uint64_t)rank, M, RANGE_START, RANGE_END);

  printf("A_%d = [", rank);
  for (int i = 0; i < N; i++) {
    printf("%f ", A[i]);
  }
  printf("]\n");

  printf("B_%d = [", rank);
  for (int i = 0; i < M; i++) {
    printf("%f ", B[i]);
  }
  printf("]\n");

  double t = -MPI_Wtime();
  auto G = std::make_unique<double[]>(N * M);

  for (int i = 0; i < N; i++) {
    for (int j = 0; j < M; j++) {
      G[i * N + j] = A[i] * B[j];
    }
  }

  auto result = std::make_unique<double[]>(N * M);
  MPI_Allreduce(G.get(), result.get(), N * M, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);

  t += MPI_Wtime();

  if (is_root) {
    printf("result:\n");
    for (int i = 0; i < N; i++) {
      for (int j = 0; j < M; j++) {
        printf("%f ", result[i * N + j]);
      }
      printf("\n");
    }
    printf("time: %f\n", t);
  }
  MPI_Finalize();
}
