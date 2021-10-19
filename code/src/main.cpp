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
#include "vector.h"

#define RANGE_START -1.0
#define RANGE_END 1.0

static const std::vector<std::string> implementations{"allreduce"};

static void print_usage(const char* exec) {
  fprintf(stderr, "Usage: %s -n N -m M [-v] -i name\n", exec);
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
    fprintf(stderr, "Invalid implementation '%s', available implementations: [", impl.c_str());

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

  auto a_vec = get_random_vectors(0, N, numprocs);
  auto b_vec = get_random_vectors(1, M, numprocs);

  if (is_root) {
    std::cout << "A" << std::endl;
    for (int i = 0; i < numprocs; i++) {
      a_vec->at(i).print();
      std::cout << std::endl;
    }

    std::cout << "B" << std::endl;
    for (int i = 0; i < numprocs; i++) {
      b_vec->at(i).print();
      std::cout << std::endl;
    }
  }

  const auto COMM = MPI_COMM_WORLD;

  auto x = impls::allreduce::allreduce(COMM, rank, numprocs);
  x.load(a_vec, b_vec);

  double t = -MPI_Wtime();
  auto result = x.compute();
  t += MPI_Wtime();

  if (is_root) {
    printf("result:\n");
    result->print();
    printf("time: %f\n", t);
  }

  MPI_Finalize();

  return EXIT_SUCCESS;
}
