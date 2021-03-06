#ifndef CODE_VECTOR_H
#define CODE_VECTOR_H

#include <string>
#include <vector>

#include "common.h"

typedef std::vector<double> vector_t;

typedef std::vector<double> vector_t;

class vector : public vector_t {
  using vector_t::vector_t; // inherit constructors

 public:
  void print() const;
  std::string string() const;
};

#endif // CODE_VECTOR_H
