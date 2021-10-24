#include <mpi.h>
#include <unistd.h>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <iterator>
#include <memory>
#include <type_traits>
#include <vector>

#include "allreduce/impl.hpp"
#include "dsop_single.h"
#include "util.hpp"
#include "vector.h"

#define ROOT 0

#define EPS 1e-5

#define TAG_BASE 123
#define TAG_VALIDATE TAG_BASE + 1
#define TAG_TIMING TAG_BASE + 2

static void print_usage(const char* exec) {
  fprintf(stderr, "Usage: %s -n N -m M [-v] [-q] [-h] -i name\n", exec);
  fprintf(stderr, "\n");
  fprintf(stderr, "  -h        Display this help and exit\n");
  fprintf(stderr, "  -v        Validate run against sequential implementation\n");
  fprintf(stderr, "  -q        Be quiet. Will not print vectors and matrices\n");
  fprintf(stderr, "  -n        Size of vector A\n");
  fprintf(stderr, "  -m        Size of vector B\n");
  fprintf(stderr, "  -i        Name of implementation to run\n");
}

template <typename... Args>
static std::unique_ptr<dsop> get_impl(const std::string& name, Args&&... args) {
  if (name == "allreduce") {
    return std::make_unique<impls::allreduce::allreduce>(std::forward<Args>(args)...);
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

  bool quiet = false;

  std::string name;
  bool has_impl;

  int opt;
  while ((opt = getopt(argc, argv, "hn:m:vi:q")) != -1) {
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
      case 'q':
        quiet = true;
        break;
      case 'h':
        print_usage(argv[0]);
        return EXIT_SUCCESS;
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

  bool is_root = rank == ROOT;

  auto impl = get_impl(name, COMM, rank, numprocs);

  printf("%d: Starting numprocs=%d N=%d, M=%d impl=%s\n", rank, numprocs, N, M, name.c_str());

  auto a_vec = get_random_vectors(0, N, numprocs);
  auto b_vec = get_random_vectors(1, M, numprocs);

  if (is_root && !quiet) {
    for (int i = 0; i < numprocs; i++) {
      std::cout << "A_" << i << " ";
      a_vec[i].print();
      std::cout << std::endl;
    }

    for (int i = 0; i < numprocs; i++) {
      std::cout << "B_" << i << " ";
      b_vec[i].print();
      std::cout << std::endl;
    }
  }

  impl->load(a_vec, b_vec);

  int64_t t;
  auto result = timer<matrix>([&]() { return impl->compute(); }, t);

  if (is_root) {
    std::vector<int64_t> timings(numprocs);

    for (int i = 0; i < numprocs; i++) {
      if (i == rank) {
        timings[i] = t;
      } else {
        MPI_Recv(&timings[i], 1, MPI_INT64_T, i, TAG_TIMING, COMM, MPI_STATUS_IGNORE);
      }
    }

    if (!quiet) {
      printf("result:\n");
      result.print();

      for (int i = 0; i < numprocs; i++) {
        printf("time %d: %f\n", i, timings[i] / 1e6);
      }
    }
    printf("time: %f\n", t / 1e6);
  } else {
    MPI_Send(&t, 1, MPI_INT64_T, ROOT, TAG_TIMING, COMM);
  }

  if (validate) {
    int num_elements = result.dimension();

    if (is_root) {
      std::cout << "Validation turned on" << std::endl;
      std::vector<matrix> results(numprocs, matrix{result.rows, result.columns});
      std::vector<MPI_Request> reqs;
      for (int i = 0; i < numprocs; i++) {
        if (i == rank) {
          results[i] = std::move(result);
        } else {
          reqs.push_back(nullptr);
          MPI_Irecv(results[i].get_ptr(), num_elements, MPI_DOUBLE, i, TAG_VALIDATE, COMM, &reqs.back());
        }
      }
      MPI_Waitall(reqs.size(), reqs.data(), MPI_STATUSES_IGNORE);

      for (int i = 1; i < numprocs; i++) {
        if (results.front() != results[i]) {
          throw std::runtime_error("Result of process " + std::to_string(i) + " differs from result of process 0");
        }
      }

      auto sequential = dsop_single();
      sequential.load(a_vec, b_vec);
      auto expected = sequential.compute();

      auto& r = results.front();

      if (!r.matchesDimensions(expected)) {
        throw std::runtime_error(
            "Results have wrong dimensions (" + std::to_string(r.rows) + "x" + std::to_string(r.columns) + ")");
      }

      double diff = nrm_sqr_diff(r.get_ptr(), expected.get_ptr(), expected.dimension());

      if (diff > EPS) {
        throw std::runtime_error("Result does not match sequential result: error=" + std::to_string(diff));
      }

    } else {
      MPI_Send(result.get_ptr(), num_elements, MPI_DOUBLE, ROOT, TAG_VALIDATE, COMM);
    }
  }

  MPI_Finalize();

  return EXIT_SUCCESS;
}
