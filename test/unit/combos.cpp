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
    EXPECT_EQ(cc.getPermutations().size(), 2);
    // No duplicates: these candidates share a non-pseudo cage
    EXPECT_EQ(cc.duplicates.none(), true);
    EXPECT_EQ(expected_masks.count(cc.combo), 1);
    expected_masks.erase(cc.combo);
  }

  // Check we've seen all masks
  EXPECT_EQ(expected_masks.empty(), true);
}

TEST_F(DefaultGridTest, ComboKillerPairs) {
  Cage cage(8);

  cage.addCell(grid.get(), Coord{0, 0});
  cage.addCell(grid.get(), Coord{1, 0});

  // {17}, {26}, {35}
  auto combos = generateCageComboInfo(&cage);

  auto killers = combos->computeKillerPairs(2);

  // No pairs
  EXPECT_EQ(killers.empty(), true);

  killers = combos->computeKillerPairs(3);
  EXPECT_EQ(killers.size(), 8);

  // {123}, {125}, {163}, {165}, {723}, {725}, {763}, {765},
  std::unordered_set<Mask> expected_killers = {
    0b000000111, 0b000010011, 0b000100101, 0b000110001,
    0b001000110, 0b001010010, 0b001100100, 0b001110000,
  };

  for (Mask const m : killers) {
    EXPECT_EQ(expected_killers.count(m), 1);
    expected_killers.erase(m);
  }

  EXPECT_EQ(expected_killers.empty(), true);
}
