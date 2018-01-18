#ifndef COLUMBO_CAGE_UNIT_OVERLAP_H
#define COLUMBO_CAGE_UNIT_OVERLAP_H

#include "debug.h"
#include "defs.h"
#include "step.h"
#include "utils.h"

struct EliminateCageUnitOverlapStep : ColumboStep {

  EliminateCageUnitOverlapStep(CageSubsetMap *subset_map) : map(subset_map) {}

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

  const char *getID() const override { return "cage-unit-overlap"; }
  const char *getName() const override { return "Cage/Unit Overlap"; }

private:
  CageSubsetMap *map;
  bool runOnHouse(House &house);
};

#endif // COLUMBO_CAGE_UNIT_OVERLAP_H
