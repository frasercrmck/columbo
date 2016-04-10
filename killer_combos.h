#ifndef COLUMBO_KILLER_COMBOS_H
#define COLUMBO_KILLER_COMBOS_H

#include "defs.h"
#include "step.h"

struct EliminateImpossibleCombosStep : ColumboStep {

  EliminateImpossibleCombosStep(CageSubsetMap *subset_map) : map(subset_map) {}

  StepCode runOnGrid(Grid *const grid) override {
    changed.clear();
    StepCode ret = {false, false};
    for (auto &cage : grid->cages) {
      ret |= runOnCage(cage);
      if (ret) {
        return ret;
      }
    }
    return ret;
  }

  virtual void anchor() override;

  const char *getName() const override { return "Removing Impossible Combos"; }

private:
  CageSubsetMap *map;
  StepCode runOnCage(Cage &cage);
};

#endif // COLUMBO_KILLER_COMBOS_H
