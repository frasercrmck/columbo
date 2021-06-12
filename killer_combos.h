#ifndef COLUMBO_KILLER_COMBOS_H
#define COLUMBO_KILLER_COMBOS_H

#include "defs.h"
#include "step.h"
#include <string_view>

struct EliminateImpossibleCombosStep : ColumboStep {

  EliminateImpossibleCombosStep() {}

  bool runOnGrid(Grid *const grid) override {
    changed.clear();
    bool modified = false;
    for (auto &cage : grid->cages) {
      modified |= runOnCage(*cage, "");
    }
    return modified;
  }

  virtual void anchor() override;

  const char *getID() const override { return "impossible-combos"; }
  const char *getName() const override { return "Removing Impossible Combos"; }
  virtual const char *getKind() const { return "Impossible Combos"; }

protected:
  bool runOnCage(Cage &cage, std::string_view dbg_reaason);
};

struct EliminateConflictingCombosStep : public EliminateImpossibleCombosStep {

  EliminateConflictingCombosStep() {}

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

  const char *getID() const override { return "conflicting-combos"; }
  const char *getName() const override { return "Removing Conflicting Combos"; }
  const char *getKind() const override { return "Conflicting Combos"; }

private:
  bool runOnHouse(House &house);
};

#endif // COLUMBO_KILLER_COMBOS_H
