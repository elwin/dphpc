#ifndef CODE_DSOP_H
#define CODE_DSOP_H

#include "common.h"
#include "matrix.h"

// dsop is the interface that dsop implementations should satisfy
class dsop {
   public:
	virtual void load(std::vector<vector>* a, std::vector<vector>* b) = 0;

	virtual matrix* compute() = 0;
};

#endif // CODE_DSOP_H
