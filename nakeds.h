#ifndef COLUMBO_NAKEDS_H
#define COLUMBO_NAKEDS_H

#include "defs.h"
#include "step.h"

struct EliminateNakedPairsStep : ColumboStep {
  bool runOnGrid(Grid *const grid) override {
    changed.clear();
    bool modified = false;
    for (auto &row : grid->rows) {
      modified |= runOnHouse(*row);
    }
    for (auto &col : grid->cols) {
      modified |= runOnHouse(*col);
    }
    for (auto &box : grid->boxes) {
      modified |= runOnHouse(*box);
    }
    return modified;
  }

  virtual void anchor() override;

  const char *getID() const override { return "naked-pairs"; }
  const char *getName() const override { return "Naked Pairs"; }

private:
  bool runOnHouse(House &house);
};

struct EliminateNakedTriplesStep : ColumboStep {
  bool runOnGrid(Grid *const grid) override {
    changed.clear();
    bool modified = false;
    for (auto &row : grid->rows) {
      modified |= runOnHouse(*row);
    }
    for (auto &col : grid->cols) {
      modified |= runOnHouse(*col);
    }
    for (auto &box : grid->boxes) {
      modified |= runOnHouse(*box);
    }
    return modified;
  }

  virtual void anchor() override;

  const char *getID() const override { return "naked-triples"; }
  const char *getName() const override { return "Naked Triples"; }

private:
  bool runOnHouse(House &house);
};

#endif // COLUMBO_NAKEDS_H
