#ifndef CODE_DSOP_SINGLE_H
#define CODE_DSOP_SINGLE_H

#include "vector.h"
#include "matrix.h"
#include "dsop.h"
#include <vector>

// The trivial single threaded version
class dsop_single : dsop {
private:
	std::vector<vector> *a{};
	std::vector<vector> *b{};

public:
	void load(std::vector<vector> *a, std::vector<vector> *b) override;

	matrix *compute() override;
};


#endif //CODE_DSOP_SINGLE_H
