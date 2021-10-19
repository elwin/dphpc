#ifndef CODE_MATRIX_H
#define CODE_MATRIX_H

#include <memory>
#include <vector>

#include "common.h"
#include "vector.h"

class matrix {
 private:
  std::unique_ptr<double[]> data;
  size_t rows;
  size_t columns;

 public:
  matrix(size_t rows, size_t columns);
  matrix(size_t rows, size_t columns, std::vector<vector>);

  void print();

  bool equal(matrix* b);

  size_t dimension() const;

  double* get_ptr();

  // Add matrix b to `this` in place
  void add(matrix* b);

  // Add matrices a and b to a new matrix
  static matrix* add(matrix* a, matrix* b);

  bool matchesDimensions(matrix* b);

  static matrix* outer(vector a, vector b);
};

#endif // CODE_MATRIX_H
