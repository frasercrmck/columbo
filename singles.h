#ifndef SOLVER_SINGLES_H
#define SOLVER_SINGLES_H

#include "defs.h"
#include "debug.h"

static bool propagateFixedCells(House &house) {
  bool modified = false;
  unsigned long fixeds_mask = 0;
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
    const unsigned long intersection = candidates->to_ulong() & fixeds_mask;
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

static bool propagateFixedCells(HouseArray &rows, HouseArray &cols,
                                HouseArray &boxes) {
  bool modified = false;
  for (auto &row : rows) {
    modified |= propagateFixedCells(*row);
  }
  for (auto &col : cols) {
    modified |= propagateFixedCells(*col);
  }
  for (auto &box : boxes) {
    modified |= propagateFixedCells(*box);
  }
  return modified;
}

#endif // SOLVER_SINGLES_H
