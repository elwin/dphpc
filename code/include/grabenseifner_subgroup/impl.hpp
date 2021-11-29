#pragma once

#include <dsop.h>
#include <mpi.h>

#include <memory>

#include "vector.h"

namespace impls::grabenseifner_subgroup {

class grabenseifner_subgroup : public dsop {
  using dsop::dsop;

 public:
  void compute(const std::vector<vector>& a_in, const std::vector<vector>& b_in, matrix& result) override;
};

extern int SUBGROUP_N_GROUPS;

} // namespace impls::grabenseifner_subgroup
