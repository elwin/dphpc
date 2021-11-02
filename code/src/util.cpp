#include "util.hpp"

#include <mpi.h>

#include <iostream>
#include <random>

#include "vector.h"

std::vector<vector> get_random_vectors(uint64_t seed, int n, int p) {
  std::mt19937_64 gen{seed};
  std::uniform_real_distribution<double> dist(-1, 1);

  std::vector<vector> out(p);

  for (int i = 0; i < p; i++) {
    for (int j = 0; j < n; j++) {
      out[i].push_back(dist(gen));
    }
  }

  return out;
}

int64_t timer_run(std::function<void(void)> fun) {
  timer t;
  fun();
  return t.finish();
}
