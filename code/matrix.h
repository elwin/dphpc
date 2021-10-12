#ifndef CODE_MATRIX_H
#define CODE_MATRIX_H


#include "vector.h"
#include <vector>

typedef std::vector<vector> matrix_t;

class matrix : public matrix_t {
	using matrix_t::matrix_t;

public:
	void print();

	bool equal(matrix *b);

	// Add matrix b to `this` in place
	void add(matrix *b);
};

matrix *outer(vector *a, vector *b);

// Add matrices a and b to a new matrix
matrix *add(matrix *a, matrix *b);

#endif //CODE_MATRIX_H