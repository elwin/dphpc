#pragma once

#include <memory>

std::unique_ptr<double[]> get_random(uint64_t seed, int size, double range_start, double range_end);
