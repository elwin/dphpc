#include "util.hpp"

#include <iostream>
#include <random>

#include "vector.h"

std::unique_ptr<double[]> get_random(uint64_t seed, int size, double range_start, double range_end) {
  std::mt19937_64 gen{seed};
  std::uniform_real_distribution<double> dist(range_start, range_end);

  auto v = std::make_unique<double[]>(size);

  for (int i = 0; i < size; i++) {
    v[i] = dist(gen);
  }

  return v;
}

std::vector<vector> get_random_vectors(uint64_t seed, int n, int p) {
  std::mt19937_64 gen{seed};
  std::uniform_real_distribution<double> dist(-1, 1);

  std::vector<vector> out(p);

  for (int i = 0; i < p; i++) {
    out[i] = vector(n);

    for (int j = 0; j < n; j++) {
      out[i][j] = dist(gen);
    }
  }

  return out;
}
