#include "framework.h"

TEST_F(DefaultGridTest, DefaultGridProperties) {
  EXPECT_EQ(grid->cols.size(), 9);
  EXPECT_EQ(grid->rows.size(), 9);
  EXPECT_EQ(grid->boxes.size(), 9);
  EXPECT_EQ(grid->cages.size(), 81);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
