#include <mpi.h>
#include <unistd.h>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <iterator>
#include <memory>
#include <vector>

#include "allreduce/impl.hpp"
#include "util.hpp"

#define RANGE_START -1.0
#define RANGE_END 1.0

static const std::vector<std::string> implementations{"allreduce"};

static void print_usage(const char* exec) {
  fprintf(stderr, "Usage: %s -n N -M m [-v] -i name\n", exec);
}

int main(int argc, char* argv[]) {
  bool validate = false;
  int N = -1;
  int M = -1;

  bool has_N = false;
  bool has_M = false;

  std::string impl;
  bool has_impl;

  int opt;
  while ((opt = getopt(argc, argv, "n:m:vi:")) != -1) {
    switch (opt) {
      case 'n':
        has_N = true;
        N = std::stoi(optarg);
        break;
      case 'm':
        has_M = true;
        M = std::stoi(optarg);
        break;
      case 'v':
        validate = true;
        break;
      case 'i':
        has_impl = true;
        impl = std::string(optarg);
        break;
      default: /* '?' */
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }
  }

  if (!has_N || !has_M || !has_impl) {
    print_usage(argv[0]);
    return EXIT_FAILURE;
  }

  assert(N > 0);
  assert(M > 0);

  auto it = std::find(implementations.begin(), implementations.end(), impl);

  if (it == implementations.end()) {
    fprintf(stderr, "Invalid implementation '%s', available implemenations: [", impl.c_str());

    for (const auto& s : implementations) {
      fprintf(stderr, "%s, ", s.c_str());
    }

    fprintf(stderr, "]\n");

    return EXIT_FAILURE;
  }

  int rank;
  int numprocs;

  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  bool is_root = rank == 0;

  printf("%d: Starting numprocs=%d N=%d, M=%d impl=%s\n", rank, numprocs, N, M, impl.c_str());

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

  const auto COMM = MPI_COMM_WORLD;

  double t = -MPI_Wtime();
  auto result = impls::allreduce::run(COMM, std::move(A), N, std::move(B), M, rank, numprocs);
  t += MPI_Wtime();

  if (is_root) {
    printf("result:\n");
    for (int i = 0; i < N; i++) {
      for (int j = 0; j < M; j++) {
        printf("%f ", result[i * M + j]);
      }
      printf("\n");
    }
    printf("time: %f\n", t);
  }
  MPI_Finalize();
}
