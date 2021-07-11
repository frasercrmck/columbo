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

TEST_F(DefaultGridTest, CageMax) {
  Cage cage(0, true);

  cage.addCell(grid.get(), Coord{0, 0});
  cage.addCell(grid.get(), Coord{1, 0});

  EXPECT_EQ(cage.getMaxValue(), 17);
}

TEST_F(DefaultGridTest, CageMax2) {
  Cage cage(0, true);

  cage.addCell(grid.get(), Coord{0, 0});
  cage.addCell(grid.get(), Coord{0, 2});
  cage.addCell(grid.get(), Coord{3, 2});

  EXPECT_EQ(cage.getMaxValue(), 26);
}

TEST_F(DefaultGridTest, CageSplitTest) {
  Cage cage(0, true);

  cage.addCell(grid.get(), Coord{2, 5});
  cage.addCell(grid.get(), Coord{1, 5});
  cage.addCell(grid.get(), Coord{0, 5});
  cage.addCell(grid.get(), Coord{0, 4});
  cage.addCell(grid.get(), Coord{0, 3});
  cage.addCell(grid.get(), Coord{4, 8});
  cage.addCell(grid.get(), Coord{4, 7});
  cage.addCell(grid.get(), Coord{4, 6});
  cage.addCell(grid.get(), Coord{3, 7});
  cage.addCell(grid.get(), Coord{3, 8});

  // Two independent groups of 5: {12345}+{12345}
  EXPECT_EQ(cage.getMinValue(), 30);
  // Two independent groups of 5: {56789}+{56789}
  EXPECT_EQ(cage.getMaxValue(), 70);
}

TEST_F(DefaultGridTest, CageSplitTest2) {
  Cage cage(0, true);

  cage.addCell(grid.get(), Coord{4, 0});
  cage.addCell(grid.get(), Coord{4, 1});
  cage.addCell(grid.get(), Coord{5, 1});
  cage.addCell(grid.get(), Coord{6, 1});
  cage.addCell(grid.get(), Coord{7, 0});
  cage.addCell(grid.get(), Coord{5, 0});
  cage.addCell(grid.get(), Coord{8, 5});
  cage.addCell(grid.get(), Coord{7, 3});
  cage.addCell(grid.get(), Coord{6, 3});
  cage.addCell(grid.get(), Coord{8, 3});
  cage.addCell(grid.get(), Coord{8, 4});

  EXPECT_EQ(cage.getMinValue(), 28);
  EXPECT_EQ(cage.getMaxValue(), 82);
}
