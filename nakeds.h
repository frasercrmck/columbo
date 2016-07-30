#ifndef COLUMBO_NAKEDS_H
#define COLUMBO_NAKEDS_H

#include "defs.h"
#include "step.h"

struct EliminateNakedPairsStep : ColumboStep {
  StepCode runOnGrid(Grid *const grid) override {
    changed.clear();
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

  const char *getID() const override { return "naked-pairs"; }
  const char *getName() const override { return "Naked Pairs"; }

private:
  StepCode runOnHouse(House &house);
};

struct EliminateNakedTriplesStep : ColumboStep {
  StepCode runOnGrid(Grid *const grid) override {
    changed.clear();
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

  const char *getID() const override { return "naked-triples"; }
  const char *getName() const override { return "Naked Triples"; }

private:
  StepCode runOnHouse(House &house);
};

#endif // COLUMBO_NAKEDS_H
