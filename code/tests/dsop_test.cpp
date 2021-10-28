#include "dsop_single.h"
#include "matrix.h"
#include "test.h"
#include "vector.h"

TEST(DSOP_SingleTest, BasicAssertions) {
  auto a = std::vector<vector>{
      vector{1, 2, 3},
      vector{2, 4, 6},
      vector{2, 4, 6},
      vector{1, 2, 3},

  };

  auto b = std::vector<vector>{
      vector{1, 2},
      vector{2, 4},
      vector{2, 4},
      vector{1, 2},
  };

  auto expected = matrix(3, 2, {{10, 20}, {20, 40}, {30, 60}});

  auto calculator = dsop_single(nullptr, 0, 1, 3, 2);
  auto result = matrix(3, 2);
  calculator.compute(a, b, result);

  EXPECT_EQ(result, expected);
}
