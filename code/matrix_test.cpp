#include "gtest/gtest.h"
#include "matrix.h"
#include "vector.h"

TEST(OuterTest, BasicAssertions) {
	vector a = vector{1, 2, 3};
	vector b = vector{1, 2, 3, 4};
	matrix x = outer(a, b);

	matrix y = matrix{
			vector{1, 2, 3, 4},
			vector{2, 4, 6, 8},
			vector{3, 6, 9, 12},
	};

	EXPECT_TRUE(x.equal(y));
}