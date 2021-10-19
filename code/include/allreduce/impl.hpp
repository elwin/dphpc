#pragma once

#include <dsop.h>
#include <mpi.h>

#include <memory>

#include "vector.h"

namespace impls::allreduce {

class allreduce : dsop {
  using dsop::dsop;

 private:
  vector a;
  vector b;

 public:
  // todo why tf are those entire declarations even needed?
  void load(std::vector<vector>* a, std::vector<vector>* b) override;
  matrix* compute() override;
};

} // namespace impls::allreduce
