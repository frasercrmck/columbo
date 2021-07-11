#include "framework.h"
#include "combinations.h"

TEST_F(DefaultGridTest, ComboBasics) {
  Cage cage(8);

  cage.addCell(grid.get(), Coord{0, 0});
  cage.addCell(grid.get(), Coord{1, 0});
  auto combos = generateCageComboInfo(&cage);

  ASSERT_EQ(&cage, combos->cage);
  ASSERT_EQ(combos->size(), combos->getCombos().size());

  // 0b01000001 {17}
  // 0b00100010 {26}
  // 0b00010100 {35}
  std::unordered_set<Mask> expected_masks = {
    0b01000001, 0b00100010, 0b00010100
  };

  ASSERT_EQ(combos->size(), expected_masks.size());

  for (CageCombo const &cc : *combos) {
    // Each has two: [17] & [71], etc
    EXPECT_EQ(cc.permutations.size(), 2);
    // No duplicates: these candidates share a non-pseudo cage
    EXPECT_EQ(cc.duplicates.none(), true);
    EXPECT_EQ(expected_masks.count(cc.combo), 1);
    expected_masks.erase(cc.combo);
  }

  // Check we've seen all masks
  EXPECT_EQ(expected_masks.empty(), true);
}

