#ifndef COLUMBO_INNIES_OUTIES_H
#define COLUMBO_INNIES_OUTIES_H

#include "defs.h"
#include "step.h"

#include <memory>
#include <algorithm>

struct EliminateOneCellInniesAndOutiesStep : ColumboStep {
  bool runOnGrid(Grid *const grid) override {
    changed.clear();
    bool modified = false;
    auto innies_and_outies = &grid->innies_and_outies;
    std::vector<InnieOutieRegion *> to_remove;

    for (auto &region : *innies_and_outies) {
      modified |= runOnRegion(*region, to_remove);
    }

    // Remove uninteresting innie & outie regions
    while (!to_remove.empty()) {
      auto *ptr = to_remove.back();
      to_remove.pop_back();

      auto iter =
          std::remove_if(innies_and_outies->begin(), innies_and_outies->end(),
                         [ptr](auto &p) { return &*p == ptr; });

      innies_and_outies->erase(iter, innies_and_outies->end());
    }

    return modified;
  }

  virtual void anchor() override;

  const char *getID() const override { return "innies-outies"; }
  const char *getName() const override { return "Innies & Outies (One Cell)"; }

protected:
  bool reduceCombinations(const InnieOutieRegion &region, const Cage &cage,
                          unsigned sum, const char *cage_type, unsigned sum_lhs,
                          unsigned sum_rhs);

private:
  virtual bool runOnRegion(InnieOutieRegion &region,
                           std::vector<InnieOutieRegion *> &to_remove);

  void performRegionMaintenance(InnieOutieRegion &region) const;
};

template <int Min, int Max>
struct EliminateMultiCellInniesAndOutiesStep
    : public EliminateOneCellInniesAndOutiesStep {

  const char *getID() const override = 0;
  const char *getName() const override = 0;

private:
  bool runOnRegion(InnieOutieRegion &region,
                   std::vector<InnieOutieRegion *> &to_remove) override;
};

struct EliminateHardInniesAndOutiesStep
    : public EliminateMultiCellInniesAndOutiesStep<2, 7> {
  const char *getID() const override { return "innies-outies-hard"; };
  const char *getName() const override {
    return "Innies & Outies (Multi-Cell)";
  }
};

template <int Min, int Max>
bool EliminateMultiCellInniesAndOutiesStep<Min, Max>::runOnRegion(
    InnieOutieRegion &region, std::vector<InnieOutieRegion *> &to_remove) {
  {
    Cage pseudo_cage;
    for (auto &io : region.innies_outies) {
      for (auto *c : io.inside_cage) {
        pseudo_cage.cells.push_back(c);
      }
    }
    if (!pseudo_cage.empty()) {
      if (region.known_cage.sum >= region.expected_sum)
        throw invalid_grid_exception{"invalid set of innies"};
      if (pseudo_cage.size() >= Min && pseudo_cage.size() <= Max) {
        unsigned sum = region.expected_sum - region.known_cage.sum;
        if (reduceCombinations(region, pseudo_cage, sum, "innie",
                               region.expected_sum, region.known_cage.sum))
          return true;
      }
    }
  }

  {
    Cage pseudo_cage;
    unsigned outie_cage_sum = 0;
    for (auto &io : region.innies_outies) {
      outie_cage_sum += io.sum;
      for (auto *c : io.outside_cage) {
        pseudo_cage.cells.push_back(c);
      }
    }
    if (!pseudo_cage.empty()) {
      if (region.known_cage.sum + outie_cage_sum <= region.expected_sum)
        throw invalid_grid_exception{"invalid set of outies"};
      if (pseudo_cage.size() >= Min && pseudo_cage.size() <= Max) {
        unsigned sum =
            region.known_cage.sum + outie_cage_sum - region.expected_sum;
        if (reduceCombinations(region, pseudo_cage, sum, "outie",
                               region.known_cage.sum + outie_cage_sum,
                               region.expected_sum))
          return true;
      }
    }
  }

  if (region.known_cage.size() == static_cast<std::size_t>(region.num_cells)) {
    to_remove.push_back(&region);
  }

  return false;
}

#endif // COLUMBO_INNIES_OUTIES_H
