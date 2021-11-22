#ifndef CODE_MATRIX_H
#define CODE_MATRIX_H

#include <cassert>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "common.h"
#include "vector.h"

class matrix {
 private:
  std::unique_ptr<double[]> data;

 public:
  matrix(size_t rows, size_t columns) : rows(rows), columns(columns) {
    this->data = std::make_unique<double[]>(rows * columns);
  }

  matrix(size_t rows, size_t columns, std::vector<vector> data) : matrix(rows, columns) {
    for (size_t i = 0; i < rows; i++) {
      for (size_t j = 0; j < columns; j++) {
        get(i, j) = data[i][j];
      }
    }
  }

  matrix(const matrix& other) : matrix(other.rows, other.columns) {
    memcpy(get_ptr(), other.get_ptr(), other.dimension() * sizeof(double));
  }

  matrix& operator=(const matrix& other) {
    data = std::make_unique<double[]>(other.dimension());
    rows = other.rows;
    columns = other.columns;
    memcpy(get_ptr(), other.get_ptr(), other.dimension() * sizeof(double));
    return *this;
  }

  size_t rows;
  size_t columns;

  void print() const {
    for (size_t i = 0; i < rows; i++) {
      for (size_t j = 0; j < columns; j++) {
        std::cerr << get(i, j) << " ";
      }

      std::cerr << std::endl;
    }
  }

  std::string string() const {
    std::string s("");
    for (size_t i = 0; i < rows; i++) {
      for (size_t j = 0; j < columns; j++) {
        s.append(std::to_string(get(i, j)));
        s.append(" ");
      }
      s.append("\n");
    }
    return s;
  }

  bool operator==(const matrix& b) const {
    if (!matchesDimensions(b))
      return false;

    for (size_t i = 0; i < rows * columns; i++) {
      if (data[i] != b.data[i])
        return false;
    }

    return true;
  }

  bool operator!=(const matrix& b) const {
    return !(*this == b);
  }

  double& get(int x, int y) const {
    return data[x * columns + y];
  }

  size_t dimension() const {
    return rows * columns;
  }

  double* get_ptr() const {
    return data.get();
  }

  // Add matrix b to `this` in place
  void add(const matrix& b) {
    assert(matchesDimensions(b));

    for (size_t i = 0; i < rows * columns; i++) {
      data[i] += b.data[i];
    }
  }

  // Add matrices a and b to a new matrix
  static matrix add(const matrix& a, const matrix& b) {
    assert(a.matchesDimensions(b));

    auto c = matrix(a.rows, a.columns);
    for (size_t i = 0; i < a.rows * a.columns; i++) {
      c.data[i] = a.data[i] + b.data[i];
    }

    return c;
  }

  bool matchesDimensions(const matrix& b) const {
    return rows == b.rows && columns == b.columns;
  }

  static matrix outer(const vector& a, const vector& b) {
    auto out = matrix(a.size(), b.size());

    for (size_t i = 0; i < a.size(); i++) {
      for (size_t j = 0; j < b.size(); j++) {
        out.get(i, j) = a[i] * b[j];
      }
    }

    return out;
  }

  // Compute outer product and write to sub-matrix in-place. Submatrix defined by start row and column.
  inline void set_submatrix_outer_product(int start_row, int start_col, const vector& a, const vector& b) {
    assert(rows >= start_row + a.size());
    assert(columns >= start_col + b.size());

    for (size_t i = 0; i < a.size(); i++) {
      for (size_t j = 0; j < b.size(); j++) {
        get(start_row + i, start_col + j) = a[i] * b[j];
      }
    }
  }

  // Compute and add the outer product and write to sub-matrix in-place. Submatrix defined by start row and column.
  inline void add_submatrix_outer_product(int start_row, int start_col, const vector& a, const vector& b) {
    assert(rows >= start_row + a.size());
    assert(columns >= start_col + b.size());

    for (size_t i = 0; i < a.size(); i++) {
      for (size_t j = 0; j < b.size(); j++) {
        get(start_row + i, start_col + j) += a[i] * b[j];
      }
    }
  }

  // Compute outer product and write to matrix in place
  inline void set_outer_product(const vector& a, const vector& b) {
    assert(dimension() == a.size() * b.size());
    set_submatrix_outer_product(0, 0, a, b);
  }

  // Compute and add the outer product and write to matrix in-place
  inline void add_outer_product(const vector& a, const vector& b) {
    assert(dimension() == a.size() * b.size());
    add_submatrix_outer_product(0, 0, a, b);
  }
};

#endif // CODE_MATRIX_H
