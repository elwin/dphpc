#include "matrix.h"

#include "gtest/gtest.h"
#include "vector.h"

TEST(OuterTest, BasicAssertions) {
  auto x = matrix::outer({1, 2, 3}, {1, 2, 3, 4});

  auto y = matrix(3, 4, {{1, 2, 3, 4}, {2, 4, 6, 8}, {3, 6, 9, 12}});

  EXPECT_EQ(x, y);
}

TEST(AddTest, BasicAssertions) {
  auto x = matrix(2, 2, {{1, 2}, {2, 4}});

  auto y = matrix(2, 2, {{3, 2}, {2, 0}});

  auto z = matrix(2, 2, {{4, 4}, {4, 4}});

  EXPECT_EQ(matrix::add(x, y), z);
}

TEST(AddTestInPlace, BasicAssertions) {
  auto x = matrix(2, 2, {{1, 2}, {2, 4}});

  auto y = matrix(2, 2, {{3, 2}, {2, 0}});
  x.add(y);

  auto z = matrix(2, 2, {{4, 4}, {4, 4}});

  EXPECT_EQ(x, z);
}
