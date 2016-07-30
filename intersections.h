#ifndef COLUMBO_INTERSECTIONS_H
#define COLUMBO_INTERSECTIONS_H

#include "debug.h"
#include "defs.h"
#include "step.h"
#include "utils.h"

struct EliminatePointingPairsOrTriplesStep : ColumboStep {
  StepCode runOnGrid(Grid *const grid) override {
    changed.clear();
    StepCode ret = {false, false};
    for (auto &row : grid->rows) {
      ret |= runOnRowOrCol(*row, grid->boxes);
      if (ret) {
        return ret;
      }
    }
    for (auto &col : grid->cols) {
      ret |= runOnRowOrCol(*col, grid->boxes);
      if (ret) {
        return ret;
      }
    }
    for (auto &box : grid->boxes) {
      ret |= runOnBox(*box, grid->rows, grid->cols);
      if (ret) {
        return ret;
      }
    }
    return ret;
  }

  virtual void anchor() override;

  const char *getID() const override { return "pointing-pairs-triples"; }
  const char *getName() const override { return "Pointing Pairs/Triples"; }

private:
  StepCode runOnRowOrCol(House &house, HouseArray &boxes);
  StepCode runOnBox(House &box, HouseArray &rows, HouseArray &cols);
};

#endif // COLUMBO_INTERSECTIONS_H
