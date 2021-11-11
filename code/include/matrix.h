#ifndef CODE_MATRIX_H
#define CODE_MATRIX_H

#include <memory>
#include <vector>

#include "common.h"
#include "vector.h"

class matrix {
 private:
  std::unique_ptr<double[]> data;

 public:
  matrix(size_t rows, size_t columns);
  matrix(size_t rows, size_t columns, std::vector<vector>);
  matrix(const matrix& other);

  matrix& operator=(const matrix& other);

  size_t rows;
  size_t columns;

  void print() const;
  std::string string() const;

  bool operator==(const matrix& b) const;
  bool operator!=(const matrix& b) const;

  double& get(int x, int y) const;

  size_t dimension() const;

  double* get_ptr() const;

  // Add matrix b to `this` in place
  void add(const matrix& b);

  // Add matrices a and b to a new matrix
  static matrix add(const matrix& a, const matrix& b);

  bool matchesDimensions(const matrix& b) const;

  static matrix outer(const vector& a, const vector& b);
};

// Compute outer product and write to matrix C in place
void set_outer_product(matrix& C, const vector& a, const vector& b);

// Compute and add the outer product and write to matrix C in-place
void add_outer_product(matrix& C, const vector& a, const vector& b);

// Compute outer product and write to sub-matrix in C in-place. Submatrix defined by start row and column.
void set_submatrix_outer_product(matrix& C, int start_row, int start_col, const vector& a, const vector& b);

// Compute and add the outer product and write to sub-matrix in C in-place. Submatrix defined by start row and column.
void add_submatrix_outer_product(matrix& C, int start_row, int start_col, const vector& a, const vector& b);

#endif // CODE_MATRIX_H
