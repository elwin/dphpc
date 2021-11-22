#include "vector.h"

#include <string>

#include "iostream"

void vector::print() const {
  std::cerr << "[";
  for (size_t i = 0; i < size(); i++) {
    if (i != 0) {
      std::cerr << ", ";
    }

    std::cerr << operator[](i);
  }
  std::cerr << "]";
}

std::string vector::string() const {
  std::string s("[");
  for (size_t i = 0; i < size(); i++) {
    if (i != 0) {
      s.append(", ");
    }
    s.append(std::to_string(operator[](i)));
  }
  s.append("]");
  return s;
}
