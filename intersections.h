#ifndef COLUMBO_INTERSECTIONS_H
#define COLUMBO_INTERSECTIONS_H

#include "debug.h"
#include "defs.h"
#include "step.h"
#include "utils.h"

struct EliminatePointingPairsOrTriplesStep : ColumboStep {
  bool runOnGrid(Grid *const grid) override {
    changed.clear();
    bool modified = false;
    for (auto &row : grid->rows) {
      modified |= runOnRowOrCol(*row, grid->boxes);
    }
    for (auto &col : grid->cols) {
      modified |= runOnRowOrCol(*col, grid->boxes);
    }
    for (auto &box : grid->boxes) {
      modified |= runOnBox(*box, grid->rows, grid->cols);
    }
    return modified;
  }

  virtual void anchor() override;

  const char *getID() const override { return "pointing-pairs-triples"; }
  const char *getName() const override { return "Pointing Pairs/Triples"; }

private:
  bool runOnRowOrCol(House &house, HouseArray &boxes);
  bool runOnBox(House &box, HouseArray &rows, HouseArray &cols);
};

#endif // COLUMBO_INTERSECTIONS_H
