#ifndef CODE_DSOP_SINGLE_H
#define CODE_DSOP_SINGLE_H

#include <vector>

#include "common.h"
#include "dsop.h"
#include "matrix.h"
#include "vector.h"

// The trivial single threaded version
class dsop_single : dsop {
 public:
  using dsop::dsop;
  matrix compute(const std::vector<vector>& a_in, const std::vector<vector>& b_in) override;
};

#endif // CODE_DSOP_SINGLE_H
