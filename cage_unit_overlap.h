#ifndef COLUMBO_CAGE_UNIT_OVERLAP_H
#define COLUMBO_CAGE_UNIT_OVERLAP_H

#include "debug.h"
#include "defs.h"
#include "step.h"
#include "utils.h"

struct EliminateCageUnitOverlapStep : ColumboStep {

  EliminateCageUnitOverlapStep() {}

  bool runOnGrid(Grid *const grid, DebugOptions const &dbg_opts) override {
    changed.clear();
    bool modified = false;
    bool debug = dbg_opts.debug(getID());
    for (auto &row : grid->rows) {
      modified |= runOnHouse(*row, debug);
    }
    for (auto &col : grid->cols) {
      modified |= runOnHouse(*col, debug);
    }
    for (auto &box : grid->boxes) {
      modified |= runOnHouse(*box, debug);
    }
    return modified;
  }

  virtual void anchor() override;

  const char *getID() const override { return "cage-unit-overlap"; }
  const char *getName() const override { return "Cage/Unit Overlap"; }

private:
  bool runOnHouse(House &house, bool debug);
};

#endif // COLUMBO_CAGE_UNIT_OVERLAP_H
