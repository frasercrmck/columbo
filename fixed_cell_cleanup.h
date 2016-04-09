#ifndef COLUMBO_FIXED_CELL_CLEANUP_H
#define COLUMBO_FIXED_CELL_CLEANUP_H

#include "debug.h"
#include "defs.h"
#include "step.h"

struct PropagateFixedCells : ColumboStep {
  StepCode runOnGrid(Grid *const grid) override {
    StepCode ret = {false, false};
    for (auto &row : grid->rows) {
      ret |= runOnHouse(*row);
      if (ret) {
        return ret;
      }
    }
    for (auto &col : grid->cols) {
      ret |= runOnHouse(*col);
      if (ret) {
        return ret;
      }
    }
    for (auto &box : grid->boxes) {
      ret |= runOnHouse(*box);
      if (ret) {
        return ret;
      }
    }
    return ret;
  }

  virtual void anchor() override;

  const char *getName() const override { return "Cleaning Up"; }

private:
  StepCode runOnHouse(House &house);
};

#endif // COLUMBO_FIXED_CELL_CLEANUP_H
