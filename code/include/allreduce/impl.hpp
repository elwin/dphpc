#pragma once

#include <dsop.h>
#include <mpi.h>

#include <memory>

#include "vector.h"

namespace impls::allreduce {

class allreduce : public dsop {
  using dsop::dsop;

 public:
  // todo why tf are those entire declarations even needed?
  matrix compute() override;
};

} // namespace impls::allreduce
