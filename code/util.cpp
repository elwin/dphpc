#include "util.hpp"

#include <random>

std::unique_ptr<double[]> get_random(uint64_t seed, int size, double range_start, double range_end) {
  std::mt19937_64 gen{seed};
  std::uniform_real_distribution<double> dist(range_start, range_end);

  auto v = std::make_unique<double[]>(size);

  for (int i = 0; i < size; i++) {
      v[i] = dist(gen);
  }

  return v;
}
