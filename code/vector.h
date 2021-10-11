#ifndef CODE_VECTOR_H
#define CODE_VECTOR_H

#include <vector>

typedef std::vector<double> vector_t;

class vector : public vector_t {
	using vector_t::vector_t;
};

#endif //CODE_VECTOR_H