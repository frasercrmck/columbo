#include "hiddens.h"

// Search a given house for a 'single': a cell that is the only that is the
// only in the house to potentially contain a value
StepCode EliminateHiddenSinglesStep::runOnHouse(House &house) {
  bool modified = false;
  CellCountMaskArray cell_masks = collectCellCountMaskInfo(house);

  for (unsigned i = 0, e = cell_masks.size(); i < e; ++i) {
    const Mask cell_mask = cell_masks[i];
    const int bit_count = bitCount(cell_mask);
    if (bit_count == 0) {
      return {true, modified};
    }
    if (bit_count != 1) {
      continue;
    }

    Cell *cell = nullptr;
    for (unsigned x = 0; x < 9; ++x) {
      if (isOn(cell_mask, x)) {
        cell = house[x];
        break;
      }
    }

    if (cell->isFixed()) {
      continue;
    }

    if (DEBUG) {
      dbgs() << "Hidden Singles: " << cell->coord << " set to " << (i + 1)
             << "; unique in " << house.getPrintKind() << "\n";
    }

    modified = true;
    changed.insert(cell);
    const Mask mask = 1 << i;

    cell->candidates = mask;
  }

  return {false, modified};
}
