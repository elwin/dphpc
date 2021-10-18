#include "matrix.h"

#include "gtest/gtest.h"
#include "vector.h"

TEST(OuterTest, BasicAssertions) {
  auto* a = new vector{1, 2, 3};
  auto* b = new vector{1, 2, 3, 4};
  matrix* x = outer(a, b);

  auto* y = new matrix{
      vector{1, 2, 3, 4},
      vector{2, 4, 6, 8},
      vector{3, 6, 9, 12},
  };

  EXPECT_TRUE(x->equal(y));
}

TEST(AddTest, BasicAssertions) {
  auto x = new matrix{
      vector{1, 2},
      vector{2, 4},
  };

  auto y = new matrix{
      vector{3, 2},
      vector{2, 0},
  };

  auto z = new matrix{
      vector{4, 4},
      vector{4, 4},
  };

  EXPECT_TRUE(add(x, y)->equal(z));
}

TEST(AddTestInPlace, BasicAssertions) {
  auto* x = new matrix{
      vector{1, 2},
      vector{2, 4},
  };

  auto* y = new matrix{
      vector{3, 2},
      vector{2, 0},
  };

  x->add(y);

  auto* z = new matrix{
      vector{4, 4},
      vector{4, 4},
  };

  EXPECT_TRUE(x->equal(z));
}