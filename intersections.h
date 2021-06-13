#ifndef COLUMBO_INTERSECTIONS_H
#define COLUMBO_INTERSECTIONS_H

#include "debug.h"
#include "defs.h"
#include "step.h"
#include "utils.h"

struct EliminatePointingPairsOrTriplesStep : ColumboStep {
  bool runOnGrid(Grid *const grid, DebugOptions const &dbg_opts) override {
    changed.clear();
    bool modified = false;
    bool debug = dbg_opts.debug(getID());
    for (auto &row : grid->rows) {
      modified |= runOnRowOrCol(*row, grid->boxes, debug);
    }
    for (auto &col : grid->cols) {
      modified |= runOnRowOrCol(*col, grid->boxes, debug);
    }
    for (auto &box : grid->boxes) {
      modified |= runOnBox(*box, grid->rows, grid->cols, debug);
    }
    return modified;
  }

  virtual void anchor() override;

  const char *getID() const override { return "pointing-pairs-triples"; }
  const char *getName() const override { return "Pointing Pairs/Triples"; }

private:
  bool runOnRowOrCol(House &house, HouseArray &boxes, bool debug);
  bool runOnBox(House &box, HouseArray &rows, HouseArray &cols, bool debug);
};

#endif // COLUMBO_INTERSECTIONS_H
