#ifndef COLUMBO_KILLER_COMBOS_H
#define COLUMBO_KILLER_COMBOS_H

#include "defs.h"
#include "step.h"
#include <string_view>

struct EliminateImpossibleCombosStep : ColumboStep {

  EliminateImpossibleCombosStep() {}

  bool runOnGrid(Grid *const grid, DebugOptions const &dbg_opts) override {
    changed.clear();
    bool modified = false;
    bool debug = dbg_opts.debug(getID());
    for (auto &cage : grid->cages)
      modified |= runOnCage(*cage, debug, "");
    return modified;
  }

  virtual void anchor() override;

  const char *getID() const override { return "impossible-combos"; }
  const char *getName() const override { return "Removing Impossible Combos"; }
  virtual const char *getKind() const { return "Impossible Combos"; }

protected:
  bool runOnCage(Cage &cage, bool debug, std::string_view dbg_reaason);
};

struct EliminateConflictingCombosStep : public EliminateImpossibleCombosStep {

  EliminateConflictingCombosStep() {}

  bool runOnGrid(Grid *const grid, DebugOptions const &dbg_opts) override {
    changed.clear();
    bool modified = false;
    bool debug = dbg_opts.debug(getID());
    for (auto &row : grid->rows)
      modified |= runOnHouse(*row, debug);
    for (auto &col : grid->cols)
      modified |= runOnHouse(*col, debug);
    for (auto &box : grid->boxes)
      modified |= runOnHouse(*box, debug);
    return modified;
  }

  virtual void anchor() override;

  const char *getID() const override { return "conflicting-combos"; }
  const char *getName() const override { return "Removing Conflicting Combos"; }
  const char *getKind() const override { return "Conflicting Combos"; }

protected:
  virtual bool runOnHouse(House &house, bool debug);
};

struct EliminateHardConflictingCombosStep : public EliminateConflictingCombosStep {

  EliminateHardConflictingCombosStep() {}

  virtual void anchor() override;

  const char *getID() const override { return "conflicting-combos-hard"; }
  const char *getName() const override {
    return "Removing Conflicting Combos (Hard)";
  }
  const char *getKind() const override { return "Conflicting Combos (Hard)"; }

private:
  bool runOnHouse(House &house, bool debug) override;
};

#endif // COLUMBO_KILLER_COMBOS_H
