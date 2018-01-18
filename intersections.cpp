#include "intersections.h"

bool EliminatePointingPairsOrTriplesStep::runOnRowOrCol(House &house,
                                                        HouseArray &boxes) {
  bool modified = false;
  CellCountMaskArray cell_masks = collectCellCountMaskInfo(house);

  for (unsigned i = 0, e = cell_masks.size(); i < e; ++i) {
    const Mask cell_mask = cell_masks[i];
    const auto bit_count = cell_mask.count();
    if (bit_count != 2 && bit_count != 3) {
      continue;
    }

    const Mask box_1_mask = 0b000000111;
    const Mask box_2_mask = 0b000111000;
    const Mask box_3_mask = 0b111000000;

    unsigned aligned_to_box = 0;
    if ((box_1_mask & cell_mask) == cell_mask) {
      aligned_to_box = 1;
    } else if ((box_2_mask & cell_mask) == cell_mask) {
      aligned_to_box = 2;
    } else if ((box_3_mask & cell_mask) == cell_mask) {
      aligned_to_box = 3;
    }

    if (!aligned_to_box) {
      continue;
    }

    std::array<Cell *, 3> cells = {{nullptr, nullptr, nullptr}};
    unsigned idx = 0;
    for (unsigned x = 0; x < 9; ++x) {
      if (!cell_mask[x]) {
        continue;
      }

      cells[idx++] = house[x];
    }

    const unsigned box_no =
        (cells[0]->coord.col / 3) + (3 * (cells[0]->coord.row / 3));

    House *box = boxes[box_no].get();

    int removed = 0;
    const Mask mask = 1 << i;
    for (Cell *cell : *box) {
      if (std::find(cells.begin(), cells.end(), cell) != cells.end()) {
        continue;
      }

      if (updateCell(cell, ~mask)) {
        modified = true;
        if (DEBUG) {
          if (!removed++) {
            dbgs() << "Pointing " << (bit_count == 2 ? "Pair" : "Triple") << " "
                   << printCellMask(house, cell_mask) << " of "
                   << house.getPrintKind() << " " << getHousePrintNum(house)
                   << ", value " << i + 1 << ", is"
                   << " aligned to box " << box_no << "; removing " << i + 1
                   << " from ";
          } else {
            dbgs() << ",";
          }
          dbgs() << cell->coord;
        }
      }
    }

    if (DEBUG && removed) {
      dbgs() << "\n";
    }
  }

  return modified;
}

bool EliminatePointingPairsOrTriplesStep::runOnBox(House &box, HouseArray &rows,
                                                   HouseArray &cols) {
  bool modified = false;
  CellCountMaskArray cell_masks = collectCellCountMaskInfo(box);

  for (unsigned i = 0, e = cell_masks.size(); i < e; ++i) {
    const Mask cell_mask = cell_masks[i];
    const auto bit_count = cell_mask.count();
    if (bit_count != 2 && bit_count != 3) {
      continue;
    }

    const Mask col_1_mask = 0b001001001;
    const Mask col_2_mask = 0b010010010;
    const Mask col_3_mask = 0b100100100;

    unsigned aligned_to_col = 0;
    if ((col_1_mask & cell_mask) == cell_mask) {
      aligned_to_col = 1;
    } else if ((col_2_mask & cell_mask) == cell_mask) {
      aligned_to_col = 2;
    } else if ((col_3_mask & cell_mask) == cell_mask) {
      aligned_to_col = 3;
    }

    const Mask row_1_mask = 0b000000111;
    const Mask row_2_mask = 0b000111000;
    const Mask row_3_mask = 0b111000000;

    unsigned aligned_to_row = 0;
    if ((row_1_mask & cell_mask) == cell_mask) {
      aligned_to_row = 1;
    } else if ((row_2_mask & cell_mask) == cell_mask) {
      aligned_to_row = 2;
    } else if ((row_3_mask & cell_mask) == cell_mask) {
      aligned_to_row = 3;
    }

    if (!aligned_to_row && !aligned_to_col) {
      continue;
    }

    std::array<Cell *, 3> cells = {{nullptr, nullptr, nullptr}};
    unsigned idx = 0;
    for (unsigned x = 0; x < 9; ++x) {
      if (!cell_mask[x]) {
        continue;
      }

      cells[idx++] = box[x];
    }

    House *house = nullptr;

    if (aligned_to_col) {
      house = cols[cells[0]->coord.col].get();
    } else {
      house = rows[cells[0]->coord.row].get();
    }

    int removed = 0;
    const Mask mask = 1 << i;
    for (Cell *cell : *house) {
      if (std::find(cells.begin(), cells.end(), cell) != cells.end()) {
        continue;
      }

      if (updateCell(cell, ~mask)) {
        modified = true;
        if (DEBUG) {
          if (!removed++) {
            dbgs() << "Pointing " << (bit_count == 2 ? "Pair" : "Triple") << " "
                   << printCellMask(box, cell_mask) << " (value " << i + 1
                   << ") is aligned to " << house->getPrintKind() << " "
                   << getHousePrintNum(*house) << "; removing " << i + 1
                   << " from ";
          } else {
            dbgs() << ",";
          }
          dbgs() << cell->coord;
        }
      }
    }
    if (DEBUG && removed) {
      dbgs() << "\n";
    }
  }

  return modified;
}
