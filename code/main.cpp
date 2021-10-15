#include "matrix.h"
#include "vector.h"

int main() {
	auto* a = new vector{1, 2, 3};
	auto* b = new vector{1, 2, 3, 4};
	matrix* x = outer(a, b);

	x->print();

	return 0;
}
