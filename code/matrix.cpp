#include "matrix.h"
#include <iostream>

matrix *outer(vector *a, vector *b) {
	auto *out = new matrix(a->size());

	for (int i = 0; i < a->size(); i++) {
		out->at(i) = vector(b->size());

		for (int j = 0; j < b->size(); j++) {
			out->at(i).at(j) = a->at(i) * b->at(j);
		}
	}

	return out;
}

matrix *add(matrix *a, matrix *b) {
	assert(a->size() == b->size());

	auto *c = new matrix(a->size());
	for (int i = 0; i < a->size(); i++) {
		assert(a->at(i).size() == b->at(i).size());
		c->at(i) = vector(a->at(i).size());

		for (int j = 0; j < a->at(i).size(); j++) {
			c->at(i).at(j) = a->at(i).at(j) + b->at(i).at(j);
		}
	}

	return c;
}

void matrix::print() {
	for (int i = 0; i < this->size(); i++) {
		for (int j = 0; j < this->at(i).size(); j++) {
			std::cout << this->at(i).at(j) << " ";
		}

		std::cout << std::endl;
	}
}

void matrix::add(matrix *b) {
	assert(this->size() == b->size());

	for (int i = 0; i < this->size(); i++) {
		assert(this->at(i).size() == b->at(i).size());

		for (int j = 0; j < this->at(i).size(); j++) {
			this->at(i).at(j) += b->at(i).at(j);
		}
	}
}

bool matrix::equal(matrix *b) {
	if (this->size() != b->size()) return false;
	for (int i = 0; i < this->size(); i++) {
		if (this->at(i).size() != b->at(i).size()) return false;

		for (int j = 0; j < this->at(i).size(); j++) {
			if (this->at(i).at(j) != b->at(i).at(j)) return false;
		}
	}

	return true;
}
