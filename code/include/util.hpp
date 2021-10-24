#pragma once

#include <mpi.h>

#include <cmath>
#include <functional>
#include <limits>
#include <memory>

#include "vector.h"

std::vector<vector> get_random_vectors(uint64_t seed, int n, int p);

/**
 * Runs the given function and returns its result.
 *
 * Writes the runtime in microseconds into the second argument
 */
template <typename R>
R timer(std::function<R(void)> fun, int64_t& us) {
  auto start = MPI_Wtime();
  auto res = fun();
  auto finish = MPI_Wtime();
  us = static_cast<int64_t>((finish - start) * 1e6);
  return res;
}

template <typename T>
T nrm_sqr_diff(T* x, T* y, int n) {
  T nrm_sqr = 0;

  for (int i = 0; i < n; i++) {
    nrm_sqr += (x[i] - y[i]) * (x[i] - y[i]);
  }

  if (std::isnan(nrm_sqr)) {
    nrm_sqr = std::numeric_limits<T>::infinity();
  }

  return nrm_sqr;
}
