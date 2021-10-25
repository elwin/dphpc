#include "vector.h"

#include "iostream"

void vector::print() {
  std::cerr << "[";
  for (size_t i = 0; i < size(); i++) {
    if (i != 0) {
      std::cerr << ", ";
    }

    std::cerr << operator[](i);
  }
  std::cerr << "]";
}
