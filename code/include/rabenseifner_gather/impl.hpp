#pragma once

#include <dsop.h>
#include <mpi.h>

#include <memory>

#include "vector.h"

namespace impls::rabenseifner_gather {

class rabenseifner_gather : public dsop {
  using dsop::dsop;

 public:
  void compute(const std::vector<vector>& a_in, const std::vector<vector>& b_in, matrix& result) override;
};

} // namespace impls::rabenseifner_gather
