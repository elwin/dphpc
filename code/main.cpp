#include <iostream>
#include <vector>
#include "vector.h"
#include "matrix.h"

int main() {
	vector a = vector{1, 2, 3};
	vector b = vector{1, 2, 3, 4};
	matrix x = outer(a, b);

	x.print();

	return 0;
}
