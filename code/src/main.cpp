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
#include "allgather_async/impl.hpp"
#include "allreduce/impl.hpp"
#include "allreduce_ring/impl.hpp"
#include "allreduce_butterfly/impl.hpp"
#include "dsop_single.h"
#include "util.hpp"
#include "vector.h"

struct settings {
  std::string name;
  int N;
  int M;
  std::string timestamp;
  bool verbose{false};
  bool validate{false};
  int rank;
  bool is_root;
  int numprocs;
  MPI_Comm COMM;
  int num_iterations{1};
};

using json = nlohmann::json;

#define EPS 1e-5

static void print_usage(const char* exec) {
  fprintf(stderr, "Usage: %s -n N -m M [-hvc] [-t iterations] -i name\n", exec);
  fprintf(stderr, "\n");
  fprintf(stderr, "  -h        Display this help and exit\n");
  fprintf(stderr, "  -v        Verbose mode\n");
  fprintf(stderr, "  -c        Check results against sequential implementation\n");
  fprintf(stderr, "  -n        Size of vector A\n");
  fprintf(stderr, "  -m        Size of vector B\n");
  fprintf(stderr, "  -i        Name of implementation to run\n");
  fprintf(stderr, "  -t        Number of iterations (default: 1)\n");
}

static settings parse_cmdline(int argc, char** argv) {
  settings s;

  bool has_N = false;
  bool has_M = false;

  bool has_impl = false;
  int opt;
  while ((opt = getopt(argc, argv, "hn:m:vi:ct:")) != -1) {
    switch (opt) {
      case 'n':
        has_N = true;
        s.N = std::stoi(optarg);
        assert(s.N > 0);
        break;
      case 'm':
        has_M = true;
        s.M = std::stoi(optarg);
        assert(s.M > 0);
        break;
      case 't':
        s.num_iterations = std::stoi(optarg);
        assert(s.num_iterations > 0);
        break;
      case 'c':
        s.validate = true;
        break;
      case 'i':
        has_impl = true;
        s.name = std::string(optarg);
        break;
      case 'v':
        s.verbose = true;
        break;
      case 'h':
        print_usage(argv[0]);
        exit(EXIT_SUCCESS);
      default: /* '?' */
        print_usage(argv[0]);
        exit(EXIT_FAILURE);
    }
  }

  if (!has_N || !has_M || !has_impl) {
    print_usage(argv[0]);
    exit(EXIT_FAILURE);
  }

  return s;
}

settings prepare(int argc, char** argv) {
  settings s = parse_cmdline(argc, argv);

  auto itt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  std::ostringstream ss;
  ss << std::put_time(localtime(&itt), "%FT%TZ%z");
  s.timestamp = ss.str();
  s.COMM = MPI_COMM_WORLD;

  MPI_Init(&argc, &argv);
  MPI_Comm_size(s.COMM, &s.numprocs);
  MPI_Comm_rank(s.COMM, &s.rank);

  s.is_root = s.rank == ROOT;

  return s;
}

template <typename... Args>
static std::unique_ptr<dsop> get_impl(const std::string& name, Args&&... args) {
  if (name == "allreduce") {
    return std::make_unique<impls::allreduce::allreduce>(std::forward<Args>(args)...);
  } else if (name == "allreduce-butterfly") {
    return std::make_unique<impls::allreduce_butterfly::allreduce_butterfly>(std::forward<Args>(args)...);
  } else if (name == "allreduce_ring") {
    return std::make_unique<impls::allreduce::allreduce_ring>(std::forward<Args>(args)...);
  } else if (name == "allgather") {
    return std::make_unique<impls::allgather::allgather>(std::forward<Args>(args)...);
  } else if (name == "allgather-async") {
    return std::make_unique<impls::allgather_async::allgather_async>(std::forward<Args>(args)...);
  } else if (name.rfind("allreduce-native-", 0) == 0) {
    return std::make_unique<impls::allreduce::allreduce>(std::forward<Args>(args)...);
  } else {
    throw std::runtime_error("Unknown implementation '" + name + "'");
  }
}

static json prepare_json_dump(const settings& s, int iteration) {
  return {
      {"timestamp", s.timestamp},
      {"name", s.name},
      {"N", s.N},
      {"M", s.M},
      {"numprocs", s.numprocs},
      {"num_iterations", s.num_iterations},
      {"iteration", iteration},
  };
}

static void run_iteration(std::unique_ptr<dsop> impl, matrix& result, MPI_Comm COMM, const std::vector<vector>& a_vec,
    const std::vector<vector>& b_vec, json& result_dump) {
  int numprocs, rank;
  MPI_Comm_size(COMM, &numprocs);
  MPI_Comm_rank(COMM, &rank);
  bool is_root = rank == ROOT;

  // Synchronize all processes before running the implementation
  MPI_Barrier(COMM);
  int64_t t = timer_run([&]() { return impl->compute(a_vec, b_vec, result); });
  int64_t mpi_time = impl->get_mpi_time();

  // Collect all runtimes in root
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

    fprintf(stderr, "time: %fs\n", t / 1e6);

    result_dump["runtimes"] = timings;
    result_dump["runtimes_mpi"] = timings_mpi;
    result_dump["runtimes_compute"] = timings_compute;

    // Index of the slowest process
    auto slowest = std::max_element(timings.begin(), timings.end()) - timings.begin();

    result_dump["runtime"] = timings[slowest];
    result_dump["runtime_mpi"] = timings_mpi[slowest];
    result_dump["runtime_compute"] = timings_compute[slowest];
  } else {
    MPI_Send(&t, 1, MPI_INT64_T, ROOT, TAG_TIMING, COMM);
    MPI_Send(&mpi_time, 1, MPI_INT64_T, ROOT, TAG_TIMING, COMM);
  }
}

/**
 * Collect all results from other processes and compare them to the reference implementation.
 *
 * On root, returns a list of errors
 */
static std::vector<double> run_validate(const settings& s, const matrix& reference_result, matrix& result) {
  if (s.is_root) {
    std::cerr << "Validation turned on" << std::endl;

    std::vector<double> errors;

    for (int i = 0; i < s.numprocs; i++) {
      if (i != s.rank) {
        MPI_Recv(result.get_ptr(), result.dimension(), MPI_DOUBLE, i, TAG_VALIDATE, s.COMM, MPI_STATUS_IGNORE);
      }

      if (!result.matchesDimensions(reference_result)) {
        throw std::runtime_error("Results of rank " + std::to_string(i) + " have wrong dimensions (" +
                                 std::to_string(result.rows) + "x" + std::to_string(result.columns) + ")");
      }

      double diff = nrm_sqr_diff(result.get_ptr(), reference_result.get_ptr(), reference_result.dimension());

      if (diff > EPS) {
        throw std::runtime_error("Result does not match sequential result: error=" + std::to_string(diff));
      }

      errors.push_back(diff);
    }

    std::cerr << "Validation successful! error=" << *std::max_element(errors.begin(), errors.end()) << std::endl;
    return errors;
  } else {
    MPI_Send(result.get_ptr(), result.dimension(), MPI_DOUBLE, ROOT, TAG_VALIDATE, s.COMM);
  }
  return {};
}

int main(int argc, char* argv[]) {
  settings s = prepare(argc, argv);

  const std::string& name = s.name;
  int N = s.N;
  int M = s.M;
  int numprocs = s.numprocs;
  int rank = s.rank;
  int is_root = s.is_root;
  bool verbose = s.verbose;
  bool validate = s.validate;
  const auto COMM = s.COMM;
  int num_iterations = s.num_iterations;

  auto a_vec = get_random_vectors(0, N, numprocs);
  auto b_vec = get_random_vectors(1, M, numprocs);

  if (is_root && verbose) {
    for (int i = 0; i < numprocs; i++) {
      fprintf(stderr, "A_%d", i);
      a_vec[i].print();
      fprintf(stderr, "\n");
      fprintf(stderr, "B_%d", i);
      b_vec[i].print();
      fprintf(stderr, "\n");
    }
  }

  /*
   * Result of the reference implementation, only set on root if validation is turned on.
   */
  std::unique_ptr<matrix> reference_result{nullptr};

  if (is_root && validate) {
    auto sequential = dsop_single(COMM, rank, numprocs, N, M);
    reference_result = std::make_unique<matrix>(N, M);
    sequential.compute(a_vec, b_vec, *reference_result);
  }

  /*
   * Stores a result. At first for actual implementation, afterwards for other processes' result for verification.
   *
   * Initialized here to avoid re-allocation of memory.
   */
  matrix result(N, M);

  fprintf(stderr, "%d: Starting numprocs=%d N=%d, M=%d impl=%s\n", rank, numprocs, N, M, name.c_str());

  for (int iter = 0; iter < num_iterations; iter++) {
    if (is_root) {
      fprintf(stderr, "Iteration %d\n", iter);
    }

    json iter_dump = prepare_json_dump(s, iter);
    auto impl = get_impl(name, COMM, rank, numprocs, N, M);

    // Reset matrix content
    memset(result.get_ptr(), 0, result.dimension() * sizeof(*result.get_ptr()));

    run_iteration(std::move(impl), result, COMM, a_vec, b_vec, iter_dump);

    if (is_root && verbose) {
      fprintf(stderr, "result:\n");
      result.print();
    }

    if (validate) {
      const auto errors = run_validate(s, *reference_result, result);
      if (is_root) {
        iter_dump["errors"] = errors;
      }
    }

    if (is_root) {
      std::cout << iter_dump << std::endl;
    }
  }

  MPI_Finalize();

  return EXIT_SUCCESS;
}
