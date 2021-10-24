#pragma once

#include <dsop.h>
#include <mpi.h>

#include <memory>

#include "vector.h"

namespace impls::allgather {

class allgather : public dsop {
  using dsop::dsop;

 public:
  // todo why tf are those entire declarations even needed?
  matrix compute() override;
};

} // namespace impls::allgather
