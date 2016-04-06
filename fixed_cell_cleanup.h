#ifndef COLUMBO_FIXED_CELL_CLEANUP_H
#define COLUMBO_FIXED_CELL_CLEANUP_H

#include "step.h"
#include "defs.h"
#include "debug.h"

static StepCode propagateFixedCells(House &house) {
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

  return {false, modified};
}

struct PropagateFixedCells : ColumboStep {
  StepCode runOnGrid(Grid *const grid) override {
    StepCode ret = {false, false};
    for (auto &row : grid->rows) {
      ret |= propagateFixedCells(*row);
      if (ret) {
        return ret;
      }
    }
    for (auto &col : grid->cols) {
      ret |= propagateFixedCells(*col);
      if (ret) {
        return ret;
      }
    }
    for (auto &box : grid->boxes) {
      ret |= propagateFixedCells(*box);
      if (ret) {
        return ret;
      }
    }
    return ret;
  }

  virtual void anchor() override;

  const char *getName() const override { return "Cleaning Up"; }
};

#endif // COLUMBO_FIXED_CELL_CLEANUP_H
