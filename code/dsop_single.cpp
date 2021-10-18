#include "dsop_single.h"

#include <cassert>

void dsop_single::load(std::vector<vector>* a, std::vector<vector>* b) {
  this->a = a;
  this->b = b;
}

matrix* dsop_single::compute() {
  matrix* result = outer(&this->a->at(0), &this->b->at(0));
  assert(this->a->size() == this->b->size());
  for (size_t i = 1; i < this->a->size(); i++) {
    result->add(outer(&this->a->at(i), &this->b->at(i)));
  }

  return result;
}
