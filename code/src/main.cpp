#include <mpi.h>
#include <unistd.h>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <memory>
#include <nlohmann/json.hpp>
#include <sstream>
#include <type_traits>
#include <vector>

#include "allgather/impl.hpp"
#include "allreduce/impl.hpp"
#include "allreduce_butterfly//impl.hpp"
#include "dsop_single.h"
#include "util.hpp"
#include "vector.h"

using json = nlohmann::json;

#define ROOT 0

#define EPS 1e-5

#define TAG_BASE 123
#define TAG_VALIDATE TAG_BASE + 1
#define TAG_TIMING TAG_BASE + 2

static void print_usage(const char* exec) {
  fprintf(stderr, "Usage: %s -n N -m M [-hvc] -i name\n", exec);
  fprintf(stderr, "\n");
  fprintf(stderr, "  -h        Display this help and exit\n");
  fprintf(stderr, "  -v        Verbose mode\n");
  fprintf(stderr, "  -c        Check results against sequential implementation\n");
  fprintf(stderr, "  -n        Size of vector A\n");
  fprintf(stderr, "  -m        Size of vector B\n");
  fprintf(stderr, "  -i        Name of implementation to run\n");
}

template <typename... Args>
static std::unique_ptr<dsop> get_impl(const std::string& name, Args&&... args) {
  if (name == "allreduce") {
    return std::make_unique<impls::allreduce::allreduce>(std::forward<Args>(args)...);
  } else if (name == "allreduce-butterfly") {
    return std::make_unique<impls::allreduce_butterfly::allreduce_butterfly>(std::forward<Args>(args)...);
  } else if (name == "allgather") {
    return std::make_unique<impls::allgather::allgather>(std::forward<Args>(args)...);
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

  bool verbose = false;

  std::string name;
  bool has_impl = false;

  int opt;
  while ((opt = getopt(argc, argv, "hn:m:vi:c")) != -1) {
    switch (opt) {
      case 'n':
        has_N = true;
        N = std::stoi(optarg);
        break;
      case 'm':
        has_M = true;
        M = std::stoi(optarg);
        break;
      case 'c':
        validate = true;
        break;
      case 'i':
        has_impl = true;
        name = std::string(optarg);
        break;
      case 'v':
        verbose = true;
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

  auto impl = get_impl(name, COMM, rank, numprocs, N, M);

  fprintf(stderr, "%d: Starting numprocs=%d N=%d, M=%d impl=%s\n", rank, numprocs, N, M, name.c_str());

  auto itt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  std::ostringstream ss;
  ss << std::put_time(localtime(&itt), "%FT%TZ%z");
  auto timestamp = ss.str();

  auto a_vec = get_random_vectors(0, N, numprocs);
  auto b_vec = get_random_vectors(1, M, numprocs);

  if (is_root && verbose) {
    for (int i = 0; i < numprocs; i++) {
      std::cerr << "A_" << i << " ";
      a_vec[i].print();
      std::cerr << std::endl;
    }

    for (int i = 0; i < numprocs; i++) {
      std::cerr << "B_" << i << " ";
      b_vec[i].print();
      std::cerr << std::endl;
    }
  }

  auto result = matrix(N, M);
  int64_t t = timer_run([&]() { return impl->compute(a_vec, b_vec, result); });
  int64_t mpi_time = impl->get_mpi_time();

  if (is_root) {
    std::vector<int64_t> timings(numprocs);
    std::vector<int64_t> timings_mpi(numprocs);
    std::vector<int64_t> timings_compute(numprocs);

    for (int i = 0; i < numprocs; i++) {
      if (i == rank) {
        timings[i] = t;
        timings_mpi[i] = mpi_time;
      } else {
        MPI_Recv(&timings[i], 1, MPI_INT64_T, i, TAG_TIMING, COMM, MPI_STATUS_IGNORE);
        MPI_Recv(&timings_mpi[i], 1, MPI_INT64_T, i, TAG_TIMING, COMM, MPI_STATUS_IGNORE);
      }

      timings_compute[i] = timings[i] - timings_mpi[i];
    }

    if (verbose) {
      fprintf(stderr, "result:\n");
      result.print();

      for (int i = 0; i < numprocs; i++) {
        fprintf(stderr, "time %d: %f\n", i, timings[i] / 1e6);
      }
    }
    fprintf(stderr, "time: %fs\n", t / 1e6);

    json result_dump;

    result_dump["timestamp"] = timestamp;
    result_dump["name"] = name;
    result_dump["N"] = N;
    result_dump["M"] = M;
    result_dump["numprocs"] = numprocs;
    result_dump["runtimes"] = timings;
    result_dump["runtimes_mpi"] = timings_mpi;
    result_dump["runtimes_compute"] = timings_compute;

    auto slowest = std::max_element(timings.begin(), timings.end()) - timings.begin();

    result_dump["runtime"] = timings[slowest];
    result_dump["runtime_mpi"] = timings_mpi[slowest];
    result_dump["runtime_compute"] = timings_compute[slowest];

    if (validate) {
      int num_elements = result.dimension();

      std::cerr << "Validation turned on" << std::endl;
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

      /*
       * Check that all produced matrices are exactly the same
       *
       * TODO can processes get slightly different matrices?
       */
      for (int i = 1; i < numprocs; i++) {
        if (results.front() != results[i]) {
          throw std::runtime_error("Result of process " + std::to_string(i) + " differs from result of process 0");
        }
      }

      auto sequential = dsop_single(COMM, rank, numprocs, N, M);
      auto expected = matrix(N, M);
      sequential.compute(a_vec, b_vec, expected);

      auto& r = results.front();

      if (!r.matchesDimensions(expected)) {
        throw std::runtime_error(
            "Results have wrong dimensions (" + std::to_string(r.rows) + "x" + std::to_string(r.columns) + ")");
      }

      double diff = nrm_sqr_diff(r.get_ptr(), expected.get_ptr(), expected.dimension());

      if (diff > EPS) {
        throw std::runtime_error("Result does not match sequential result: error=" + std::to_string(diff));
      }

      // Since we require all matrices to be the same, the error is the same for all processes.
      result_dump["errors"] = std::vector<double>(numprocs, diff);
    }
    std::cout << result_dump << std::endl;
  } else {
    MPI_Send(&t, 1, MPI_INT64_T, ROOT, TAG_TIMING, COMM);
    MPI_Send(&mpi_time, 1, MPI_INT64_T, ROOT, TAG_TIMING, COMM);
    if (validate) {
      int num_elements = result.dimension();
      MPI_Send(result.get_ptr(), num_elements, MPI_DOUBLE, ROOT, TAG_VALIDATE, COMM);
    }
  }

  MPI_Finalize();

  return EXIT_SUCCESS;
}
