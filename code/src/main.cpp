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

static void print_usage(const char* exec) {
  fprintf(stderr, "Usage: %s -n N -m M [-v] -i name\n", exec);
}

static std::unique_ptr<dsop> get_impl(const std::string& name, MPI_Comm comm, int rank, int num_procs) {
  if (name == "allreduce") {
    return std::make_unique<impls::allreduce::allreduce>(comm, rank, num_procs);
  } else {
    throw std::runtime_error("Unknown implementation '" + name + "'");
  }
}

int main(int argc, char* argv[]) {
  bool validate = false;
  int N = -1;
  int M = -1;

  bool has_N = false;
  bool has_M = false;

  std::string name;
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
        name = std::string(optarg);
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

  int rank;
  int numprocs;
  const auto COMM = MPI_COMM_WORLD;

  MPI_Init(&argc, &argv);
  MPI_Comm_size(COMM, &numprocs);
  MPI_Comm_rank(COMM, &rank);

  bool is_root = rank == 0;

  auto impl = get_impl(name, COMM, rank, numprocs);

  printf("%d: Starting numprocs=%d N=%d, M=%d impl=%s\n", rank, numprocs, N, M, name.c_str());

  auto a_vec = get_random_vectors(0, N, numprocs);
  auto b_vec = get_random_vectors(1, M, numprocs);

  if (is_root) {
    std::cout << "A_" << rank << std::endl;
    for (int i = 0; i < numprocs; i++) {
      a_vec[i].print();
      std::cout << std::endl;
    }

    std::cout << "B_" << rank << std::endl;
    for (int i = 0; i < numprocs; i++) {
      b_vec[i].print();
      std::cout << std::endl;
    }
  }

  impl->load(a_vec, b_vec);

  double t = -MPI_Wtime();
  auto result = impl->compute();
  t += MPI_Wtime();

  if (is_root) {
    printf("result:\n");
    result.print();
    printf("time: %f\n", t);
  }

  MPI_Finalize();

  return EXIT_SUCCESS;
}
