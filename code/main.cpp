#include "matrix.h"
#include "vector.h"

int main() {
	matrix* x = matrix::outer({1, 2, 3}, {1, 2, 3, 4});

	x->print();

	return 0;
}
