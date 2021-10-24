#include "dsop_single.h"

#include <cassert>

void dsop_single::load(const std::vector<vector>& a_in, const std::vector<vector>& b_in) {
  a = a_in;
  b = b_in;
  assert(a.size() == b.size());
}

matrix dsop_single::compute() {
  matrix result = matrix::outer(a[0], b[0]);
  for (size_t i = 1; i < a.size(); i++) {
    result.add(matrix::outer(a[i], b[i]));
  }

  return result;
}
