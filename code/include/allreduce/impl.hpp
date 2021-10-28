#pragma once

#include <dsop.h>
#include <mpi.h>

#include <memory>

#include "vector.h"

namespace impls::allreduce {

class allreduce : public dsop {
  using dsop::dsop;

 public:
  matrix compute(const std::vector<vector>& a_in, const std::vector<vector>& b_in) override;
};

} // namespace impls::allreduce
