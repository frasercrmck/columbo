#ifndef COLUMBO_KILLER_COMBOS_H
#define COLUMBO_KILLER_COMBOS_H

#include "defs.h"
#include "step.h"

struct EliminateImpossibleCombosStep : ColumboStep {

  EliminateImpossibleCombosStep(CageSubsetMap *subset_map)
      : cage_combo_map(subset_map) {}

  bool runOnGrid(Grid *const grid) override {
    changed.clear();
    bool modified = false;
    for (auto &cage : grid->cages) {
      modified |= runOnCage(*cage);
    }
    return modified;
  }

  virtual void anchor() override;

  const char *getID() const override { return "impossible-combos"; }
  const char *getName() const override { return "Removing Impossible Combos"; }

private:
  bool runOnCage(Cage &cage);
  CageSubsetMap *cage_combo_map;
};

#endif // COLUMBO_KILLER_COMBOS_H
