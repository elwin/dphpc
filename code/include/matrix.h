#ifndef CODE_MATRIX_H
#define CODE_MATRIX_H

#include <vector>

#include "common.h"
#include "vector.h"

class matrix {
	//	using matrix_t::matrix_t;
   private:
	std::unique_ptr<double[]> data;
	int rows;
	int columns;

   public:
	matrix(int rows, int columns);
	matrix(int rows, int columns, std::vector<std::vector<double>>);

	void print();

	bool equal(matrix* b);

	// Add matrix b to `this` in place
	void add(matrix* b);

	// Add matrices a and b to a new matrix
	static matrix* add(matrix* a, matrix* b);

	bool matchesDimensions(matrix* b);

	static matrix* outer(std::vector<double> a, std::vector<double> b);
};

#endif // CODE_MATRIX_H
