#include "vector.h"

#include "iostream"

void vector::print() {
  std::cout << "[";
  for (size_t i = 0; i < size(); i++) {
    if (i != 0) {
      std::cout << ", ";
    }

    std::cout << operator[](i);
  }
  std::cout << "]";
}
