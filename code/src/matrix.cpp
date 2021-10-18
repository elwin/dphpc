#include "matrix.h"

#include <cassert>
#include <iostream>

matrix::matrix(size_t rows, size_t columns) {
  this->data = std::make_unique<double[]>(rows * columns);
  this->rows = rows;
  this->columns = columns;
}

matrix::matrix(size_t rows, size_t columns, std::vector<std::vector<double>> data) : matrix(rows, columns) {
  for (size_t i = 0; i < rows; i++) {
    for (size_t j = 0; j < columns; j++) {
      this->data[i * columns + j] = data.at(i).at(j);
    }
  }
}

matrix* matrix::outer(std::vector<double> a, std::vector<double> b) {
  auto* out = new matrix(a.size(), b.size());

  for (size_t i = 0; i < a.size(); i++) {
    for (size_t j = 0; j < b.size(); j++) {
      out->data[i * out->columns + j] = a.at(i) * b.at(j);
    }
  }

  return out;
}

void matrix::print() {
  for (size_t i = 0; i < this->rows; i++) {
    for (size_t j = 0; j < this->columns; j++) {
      std::cout << this->data[i * this->columns + j] << " ";
    }

    std::cout << std::endl;
  }
}

void matrix::add(matrix* b) {
  assert(this->matchesDimensions(b));

  for (size_t i = 0; i < this->rows * this->columns; i++) {
    this->data[i] += b->data[i];
  }
}

matrix* matrix::add(matrix* a, matrix* b) {
  assert(a->matchesDimensions(b));

  auto c = new matrix(a->rows, a->columns);
  for (size_t i = 0; i < a->rows * a->columns; i++) {
    c->data[i] = a->data[i] + b->data[i];
  }

  return c;
}

bool matrix::equal(matrix* b) {
  if (!this->matchesDimensions(b))
    return false;

  for (size_t i = 0; i < this->rows * this->columns; i++) {
    if (this->data[i] != b->data[i])
      return false;
  }

  return true;
}
bool matrix::matchesDimensions(matrix* b) {
  return this->rows = b->rows && this->columns == b->columns;
}
