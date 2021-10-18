#include "matrix.h"

#include "gtest/gtest.h"
#include "vector.h"

TEST(OuterTest, BasicAssertions) {
	matrix* x = matrix::outer({1, 2, 3}, {1, 2, 3, 4});

	auto* y = new matrix(3, 4, {{1, 2, 3, 4}, {2, 4, 6, 8}, {3, 6, 9, 12}});

	x->print();
	y->print();

	EXPECT_TRUE(x->equal(y));
}

TEST(AddTest, BasicAssertions) {
	auto x = new matrix(2, 2, {{1, 2}, {2, 4}});

	auto y = new matrix(2, 2, {{3, 2}, {2, 0}});

	auto z = new matrix(2, 2, {{4, 4}, {4, 4}});

	EXPECT_TRUE(matrix::add(x, y)->equal(z));
}

TEST(AddTestInPlace, BasicAssertions) {
	auto x = new matrix(2, 2, {{1, 2}, {2, 4}});

	auto y = new matrix(2, 2, {{3, 2}, {2, 0}});
	x->add(y);

	auto z = new matrix(2, 2, {{4, 4}, {4, 4}});

	EXPECT_TRUE(x->equal(z));
}
