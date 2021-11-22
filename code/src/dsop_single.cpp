#include "dsop_single.h"

#include <cassert>

void dsop_single::compute(const std::vector<vector>& a, const std::vector<vector>& b, matrix& result) {
  for (size_t i = 0; i < a.size(); i++) {
    add_outer_product(result, a[i], b[i]);
  }
}
