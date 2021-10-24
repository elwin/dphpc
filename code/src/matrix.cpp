#include "matrix.h"

#include <cassert>
#include <iostream>

matrix::matrix(size_t rows, size_t columns) {
  this->data = std::make_unique<double[]>(rows * columns);
  this->rows = rows;
  this->columns = columns;
}

double& matrix::get(int x, int y) const {
  return data[x * columns + y];
}

matrix::matrix(size_t rows, size_t columns, std::vector<vector> data) : matrix(rows, columns) {
  for (size_t i = 0; i < rows; i++) {
    for (size_t j = 0; j < columns; j++) {
      get(i, j) = data[i][j];
    }
  }
}

matrix matrix::outer(const vector& a, const vector& b) {
  auto out = matrix(a.size(), b.size());

  for (size_t i = 0; i < a.size(); i++) {
    for (size_t j = 0; j < b.size(); j++) {
      out.get(i, j) = a[i] * b[j];
    }
  }

  return out;
}

void matrix::print() const {
  for (size_t i = 0; i < rows; i++) {
    for (size_t j = 0; j < columns; j++) {
      std::cout << get(i, j) << " ";
    }

    std::cout << std::endl;
  }
}

void matrix::add(const matrix& b) {
  assert(matchesDimensions(b));

  for (size_t i = 0; i < rows * columns; i++) {
    data[i] += b.data[i];
  }
}

matrix matrix::add(const matrix& a, const matrix& b) {
  assert(a.matchesDimensions(b));

  auto c = matrix(a.rows, a.columns);
  for (size_t i = 0; i < a.rows * a.columns; i++) {
    c.data[i] = a.data[i] + b.data[i];
  }

  return c;
}

bool matrix::operator==(const matrix& b) const {
  if (!matchesDimensions(b))
    return false;

  for (size_t i = 0; i < rows * columns; i++) {
    if (data[i] != b.data[i])
      return false;
  }

  return true;
}

bool matrix::matchesDimensions(const matrix& b) const {
  return rows == b.rows && columns == b.columns;
}

size_t matrix::dimension() const {
  return rows * columns;
}

double* matrix::get_ptr() {
  return data.get();
}
