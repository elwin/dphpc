#pragma once

#include <dsop.h>
#include <mpi.h>

#include <memory>

#include "vector.h"

namespace impls::grabenseifner_allgather_scatter {

class grabenseifner_allgather_scatter : public dsop {
  using dsop::dsop;

 public:
  void compute(const std::vector<vector>& a_in, const std::vector<vector>& b_in, matrix& result) override;
};

} // namespace impls::grabenseifner_allgather
