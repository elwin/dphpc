#pragma once

#include <memory>

#include "vector.h"

std::unique_ptr<double[]> get_random(uint64_t seed, int size, double range_start, double range_end);
std::vector<vector>* get_random_vectors(uint64_t seed, int n, int p);
