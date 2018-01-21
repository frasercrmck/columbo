#ifndef COLUMBO_INNIES_OUTIES_H
#define COLUMBO_INNIES_OUTIES_H

#include "defs.h"
#include "step.h"

#include <memory>

struct EliminateOneCellInniesAndOutiesStep : ColumboStep {
  bool runOnGrid(Grid *const grid) override {
    changed.clear();
    bool modified = false;
    auto innies_and_outies = &grid->innies_and_outies;
    std::vector<std::unique_ptr<InnieOutieRegion> *> to_remove;

    for (auto &region : *innies_and_outies) {
      modified |= runOnRegion(region, to_remove);
    }

    // Remove uninteresting innie & outie regions
    while (!to_remove.empty()) {
      auto *ptr = to_remove.back();
      to_remove.pop_back();

      auto iter = std::remove(innies_and_outies->begin(),
                              innies_and_outies->end(), *ptr);

      innies_and_outies->erase(iter, innies_and_outies->end());
    }

    return modified;
  }

  virtual void anchor() override;

  const char *getID() const override { return "innies-outies"; }
  const char *getName() const override { return "Innies & Outies (One Cell)"; }

private:
  bool runOnRegion(std::unique_ptr<InnieOutieRegion> &region,
                   std::vector<std::unique_ptr<InnieOutieRegion> *> &to_remove);

  bool reduceCombinations(const InnieOutieRegion &region, const Cage &cage,
                          unsigned sum, const char *cage_type);
  bool reduceUnknownCombinations(InnieOutieRegion *region);
};

#endif // COLUMBO_INNIES_OUTIES_H
