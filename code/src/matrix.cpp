#include "matrix.h"

#include <cassert>
#include <cstring>
#include <iostream>

matrix::matrix(size_t rows, size_t columns) : rows(rows), columns(columns) {
  this->data = std::make_unique<double[]>(rows * columns);
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

matrix::matrix(const matrix& other) : matrix(other.rows, other.columns) {
  memcpy(get_ptr(), other.get_ptr(), other.dimension() * sizeof(double));
}

matrix& matrix::operator=(const matrix& other) {
  data = std::make_unique<double[]>(other.dimension());
  rows = other.rows;
  columns = other.columns;
  memcpy(get_ptr(), other.get_ptr(), other.dimension() * sizeof(double));
  return *this;
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

void set_outer_product(matrix& C, const vector& a, const vector& b) {
  assert(C.dimension() == a.size() * b.size());
  C.rows = a.size();
  C.columns = b.size();

  for (size_t i = 0; i < a.size(); i++) {
    for (size_t j = 0; j < b.size(); j++) {
      C.get(i, j) = a[i] * b[j];
    }
  }
}

void add_outer_product(matrix& C, const vector& a, const vector& b) {
  assert(C.dimension() == a.size() * b.size());
  C.rows = a.size();
  C.columns = b.size();

  for (size_t i = 0; i < a.size(); i++) {
    for (size_t j = 0; j < b.size(); j++) {
      C.get(i, j) += a[i] * b[j];
    }
  }
}

void set_submatrix_outer_product(matrix& C, int start_row, int start_col, const vector& a, const vector& b) {
  assert(C.rows >= start_row + a.size());
  assert(C.columns >= start_col + b.size());

  for (size_t i = 0; i < a.size(); i++) {
    for (size_t j = 0; j < b.size(); j++) {
      C.get(start_row + i, start_col + j) = a[i] * b[j];
    }
  }
}

void add_submatrix_outer_product(matrix& C, int start_row, int start_col, const vector& a, const vector& b) {
  assert(C.rows >= start_row + a.size());
  assert(C.columns >= start_col + b.size());

  for (size_t i = 0; i < a.size(); i++) {
    for (size_t j = 0; j < b.size(); j++) {
      C.get(start_row + i, start_col + j) += a[i] * b[j];
    }
  }
}

void matrix::print() const {
  for (size_t i = 0; i < rows; i++) {
    for (size_t j = 0; j < columns; j++) {
      std::cerr << get(i, j) << " ";
    }

    std::cerr << std::endl;
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

bool matrix::operator!=(const matrix& b) const {
  return !(*this == b);
}

bool matrix::matchesDimensions(const matrix& b) const {
  return rows == b.rows && columns == b.columns;
}

size_t matrix::dimension() const {
  return rows * columns;
}

double* matrix::get_ptr() const {
  return data.get();
}
