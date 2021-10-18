#include "dsop_single.h"
#include "matrix.h"
#include "test.h"
#include "vector.h"

TEST(DSOP_SingleTest, BasicAssertions) {
  auto a = new std::vector<vector>{
      vector{1, 2, 3},
      vector{2, 4, 6},
      vector{2, 4, 6},
      vector{1, 2, 3},

  };

  auto b = new std::vector<vector>{
      vector{1, 2},
      vector{2, 4},
      vector{2, 4},
      vector{1, 2},
  };

  auto expected = new matrix{
      vector{10, 20},
      vector{20, 40},
      vector{30, 60},
  };

  auto calculator = new dsop_single();
  calculator->load(a, b);
  matrix* result = calculator->compute();

  EXPECT_TRUE(result->equal(expected));
}
