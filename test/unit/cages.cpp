#include "framework.h"

// Two cells that share a cage. Their candidates effect one another.
TEST_F(DefaultGridTest, CageMin) {
  Cage cage(0, true);

  cage.addCell(grid.get(), Coord{0, 0});
  cage.addCell(grid.get(), Coord{1, 0});

  EXPECT_EQ(cage.getMinValue(), 3);

  cage[0]->candidates.reset(1);
  EXPECT_EQ(cage.getMinValue(), 3);

  cage[1]->candidates.reset(1);
  EXPECT_EQ(cage.getMinValue(), 4);

  cage[0]->candidates.set(1);
  EXPECT_EQ(cage.getMinValue(), 3);

  cage[1]->candidates.set(1);
  EXPECT_EQ(cage.getMinValue(), 3);
}

// Two cells that share a pseudo cage. Their candidates have no effect on one
// another.
TEST_F(DefaultGridTest, PseudoCageMin) {
  Cage cage(0, true);

  cage.addCell(grid.get(), Coord{0, 0});
  cage.addCell(grid.get(), Coord{4, 4});

  EXPECT_EQ(cage.getMinValue(), 2);

  cage[0]->candidates.reset(1);
  EXPECT_EQ(cage.getMinValue(), 2);

  cage[1]->candidates.reset(1);
  EXPECT_EQ(cage.getMinValue(), 2);

  cage[0]->candidates.set(1);
  EXPECT_EQ(cage.getMinValue(), 2);

  cage[1]->candidates.set(1);
  EXPECT_EQ(cage.getMinValue(), 2);
}
