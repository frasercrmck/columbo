#ifndef SOLVER_FIXED_CELL_CLEANUP_H
#define SOLVER_FIXED_CELL_CLEANUP_H

#include "defs.h"
#include "debug.h"

static bool propagateFixedCells(House &house) {
  bool modified = false;
  Mask fixeds_mask = 0;
  for (auto &cell : house) {
    if (cell->isFixed()) {
      fixeds_mask |= 1 << (cell->isFixed() - 1);
    }
  }

  for (auto &cell : house) {
    if (cell->isFixed()) {
      continue;
    }
    CandidateSet *candidates = &cell->candidates;
    const Mask intersection = candidates->to_ulong() & fixeds_mask;
    if (intersection != 0) {
      modified = true;
      if (DEBUG) {
        dbgs() << "Clean Up: removing " << printCandidateString(intersection)
               << " from " << cell->coord << "\n";
      }
      *candidates = CandidateSet(candidates->to_ulong() & ~fixeds_mask);
    }
  }

  return modified;
}

static bool propagateFixedCells(Grid *const grid) {
  bool modified = false;
  for (auto &row : grid->rows) {
    modified |= propagateFixedCells(*row);
  }
  for (auto &col : grid->cols) {
    modified |= propagateFixedCells(*col);
  }
  for (auto &box : grid->boxes) {
    modified |= propagateFixedCells(*box);
  }
  return modified;
}

#endif // SOLVER_FIXED_CELL_CLEANUP_H
