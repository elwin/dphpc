#include "dsop_single.h"

#include <cassert>

void dsop_single::load(std::vector<vector>* a_in, std::vector<vector>* b_in) {
  this->a = a_in;
  this->b = b_in;
}

matrix* dsop_single::compute() {
  matrix* result = matrix::outer(this->a->at(0), this->b->at(0));
  assert(this->a->size() == this->b->size());
  for (size_t i = 1; i < this->a->size(); i++) {
    result->add(matrix::outer(this->a->at(i), this->b->at(i)));
  }

  return result;
}
