#ifndef SOLVER_CAGE_UNIT_OVERLAP_H
#define SOLVER_CAGE_UNIT_OVERLAP_H

#include "defs.h"
#include "utils.h"
#include "debug.h"

bool exposeHiddenCagePairs(House &house) {
  bool modified = false;

  CellCountMaskArray cell_masks;
  collectCellCountMaskInfo(house, cell_masks);

  for (unsigned i = 0, e = cell_masks.size(); i < e; ++i) {
    const Mask cell_mask = cell_masks[i];
    if (bitCount(cell_mask) != 2) {
      continue;
    }

    // We've found a hidden triple!
    std::array<Cell *, 2> pair_cells;

    unsigned idx = 0;
    for (unsigned x = 0; x < 9; ++x) {
      const bool is_on = (cell_mask >> x) & 0x1;
      if (!is_on) {
        continue;
      }

      pair_cells[idx++] = house[x];
    }

    bool is_interesting = true;
    Cell *const cell_0 = pair_cells[0];
    Cell *const cell_1 = pair_cells[1];

    // Don't care about fixed cells
    is_interesting &= (!cell_0->isFixed() && !cell_1->isFixed());
    // Don't care about cells in disjoint cells
    is_interesting &= cell_0->cage == cell_1->cage;
    // Don't care about cages larger than 2
    is_interesting &= cell_0->cage->size() == 2;
    // Don't care about naked pairs
    is_interesting &=
        (cell_0->candidates.count() != 2 && cell_1->candidates.count() != 2);

    if (!is_interesting) {
      continue;
    }

    if (DEBUG) {
      dbgs() << "Two-cell cage " << cell_0->coord << "/" << cell_1->coord
             << " must contain " << (i + 1) << ". ";
    }

    const int other_cell = cell_0->cage->sum - static_cast<int>(i + 1);

    const Mask mask = (1 << (other_cell - 1)) | (1 << i);

    modified = true;

    if (DEBUG) {
      dbgs() << "Setting " << cell_0->coord << " and " << cell_1->coord
             << " to " << printCandidateString(mask) << "\n";
    }

    cell_0->candidates = CandidateSet(mask);
    cell_1->candidates = CandidateSet(mask);
  }

  return modified;
}

static bool exposeHiddenCagePairs(Grid *const grid) {
  bool modified = false;
  for (auto &row : grid->rows) {
    modified |= exposeHiddenCagePairs(*row);
  }
  for (auto &col : grid->cols) {
    modified |= exposeHiddenCagePairs(*col);
  }
  for (auto &box : grid->boxes) {
    modified |= exposeHiddenCagePairs(*box);
  }
  return modified;
}

#endif // SOLVER_CAGE_UNIT_OVERLAP_H
