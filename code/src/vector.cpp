#include "vector.h"

#include "iostream"

void vector::print() {
  for (size_t i = 0; i < this->size(); i++) {
    std::cout << (*this)[i] << " ";
  }
}
