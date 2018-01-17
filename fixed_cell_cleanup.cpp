#include "fixed_cell_cleanup.h"

StepCode PropagateFixedCells::runOnHouse(House &house, const Cell *fixed_cell) {
  bool modified = false;
  const Mask fixed_mask = fixed_cell->candidates;

  int removed = 0;
  for (auto &c : house) {
    // Not interested in fixed cells
    if (c == fixed_cell || c->isFixed()) {
      continue;
    }

    if (auto intersection = updateCell(c, ~fixed_mask)) {
      modified = true;
      work_list.insert(c);
      if (DEBUG) {
        if (!removed++) {
          dbgs() << "Clean Up: removing " << printCandidateString(*intersection)
                 << " from ";
        } else {
          dbgs() << ",";
        }
        dbgs() << c->coord;
      }
    }
  }

  if (DEBUG && removed) {
    dbgs() << "\n";
  }

  return {false, modified};
}
