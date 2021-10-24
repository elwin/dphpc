#ifndef CODE_DSOP_SINGLE_H
#define CODE_DSOP_SINGLE_H

#include <vector>

#include "common.h"
#include "dsop.h"
#include "matrix.h"
#include "vector.h"

// The trivial single threaded version
class dsop_single : dsop {
 private:
  std::vector<vector> a{};
  std::vector<vector> b{};

 public:
  void load(const std::vector<vector>& a, const std::vector<vector>& b) override;

  matrix compute() override;
};

#endif // CODE_DSOP_SINGLE_H
