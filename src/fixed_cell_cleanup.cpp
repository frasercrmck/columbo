#include "fixed_cell_cleanup.h"

bool PropagateFixedCells::runOnHouse(House &house, const Cell *fixed_cell, bool debug) {
  bool modified = false;
  const Mask fixed_mask = fixed_cell->candidates;

  Mask fixed_cells = fixed_mask;
  int removed = 0;
  for (auto &c : house) {
    // Not interested in fixed cells
    if (c == fixed_cell)
      continue;
    if (c->isFixed()) {
      if (fixed_cells[c->isFixed() - 1]) {
        std::stringstream ss;
        ss << "Cell value " << c->isFixed() << " fixed multiple times in "
           << house << "!";
        throw invalid_grid_exception{ss.str()};
      }
      fixed_cells.set(c->isFixed() - 1);
      continue;
    }

    if (auto intersection = updateCell(c, ~fixed_mask)) {
      modified = true;
      work_list.insert(c);
      if (debug) {
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

  if (debug && removed) {
    dbgs() << "\n";
  }

  return modified;
}
