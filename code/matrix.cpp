#include "matrix.h"
#include <iostream>

matrix outer(vector a, vector b) {
	matrix out = matrix(a.size());

	for (int i = 0; i < a.size(); i++) {
		out[i] = vector(b.size());

		for (int j = 0; j < b.size(); j++) {
			out[i][j] = a[i] * b[j];
		}
	}

	return out;
}

void matrix::print() {
	for (int i = 0; i < this->size(); i++) {
		for (int j = 0; j < this->at(i).size(); j++) {
			std::cout << this->at(i).at(j) << " ";
		}

		std::cout << std::endl;
	}
}
