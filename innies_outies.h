#ifndef COLUMBO_INNIES_OUTIES_H
#define COLUMBO_INNIES_OUTIES_H

#include "defs.h"
#include "step.h"
#include "combinations.h"
#include "debug.h"

#include <memory>
#include <algorithm>

struct EliminateOneCellInniesAndOutiesStep : ColumboStep {
  bool runOnGrid(Grid *const grid) override {
    changed.clear();
    bool modified = false;
    auto innies_and_outies = &grid->innies_and_outies;
    std::vector<InnieOutieRegion *> to_remove;

    for (auto &region : *innies_and_outies) {
      modified |= runOnRegion(grid, *region, to_remove);
    }

    // Remove uninteresting innie & outie regions
    while (!to_remove.empty()) {
      auto *ptr = to_remove.back();
      to_remove.pop_back();

      if (ptr->house && ptr->house->region) {
        if (ptr->house->region != ptr)
          throw invalid_grid_exception{"mismatching region/house"};
        ptr->house->region = nullptr;
      }

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
  virtual bool runOnRegion(Grid *const grid, InnieOutieRegion &region,
                           std::vector<InnieOutieRegion *> &to_remove);

  void performRegionMaintenance(InnieOutieRegion &region) const;
};

template <int Min, int Max>
struct EliminateMultiCellInniesAndOutiesStep
    : public EliminateOneCellInniesAndOutiesStep {

  const char *getID() const override = 0;
  const char *getName() const override = 0;

private:
  bool runOnRegion(Grid *const grid, InnieOutieRegion &region,
                   std::vector<InnieOutieRegion *> &to_remove) override;
};

struct EliminateHardInniesAndOutiesStep
    : public EliminateMultiCellInniesAndOutiesStep<2, 7> {
  const char *getID() const override { return "innies-outies-hard"; };
  const char *getName() const override {
    return "Innies & Outies (Multi-Cell)";
  }
};

static Cage *
getOrCreatePseudoCage(Grid *const grid, InnieOutieRegion &region,
                      std::vector<std::unique_ptr<Cage>> &cage_list,
                      Cage &pseudo_cage) {
  // Check whether we've already computed this cage.
  if (auto it = std::find_if(std::begin(cage_list), std::end(cage_list),
                             [&pseudo_cage](auto const &cage_ptr) {
                               return cage_ptr->sum == pseudo_cage.sum &&
                                      cage_ptr->size() == pseudo_cage.size() &&
                                      cage_ptr->member_set() ==
                                          pseudo_cage.member_set();
                             });
      it != std::end(cage_list)) {
    return it->get();
  }

  cage_list.push_back(std::make_unique<Cage>(pseudo_cage));
  Cage *the_cage = cage_list.back().get();

  // Only pre-compute the sums for the "easy" no-duplicate cases for now.
  if (the_cage->doAllCellsSeeEachOther()) {
    std::vector<Mask> possibles;
    possibles.reserve(the_cage->size());
    for (auto const *cell : the_cage->cells)
      possibles.push_back(cell->candidates);
    auto cage_combos = generateCageSubsetSums(the_cage->sum, possibles);
    // As a stop-gap, expand permutations here.
    for (CageCombo &cage_combo : cage_combos)
      expandComboPermutations(the_cage, cage_combo);
    grid->cage_combos.emplace_back(
        std::make_unique<CageComboInfo>(the_cage, std::move(cage_combos)));
    the_cage->cage_combos = grid->cage_combos.back().get();
  }

  // Finally, register this cage with all of its component cells.
  for (auto *c : *the_cage)
    c->pseudo_cages.push_back(the_cage);

  return the_cage;
}

template <int Min, int Max>
bool EliminateMultiCellInniesAndOutiesStep<Min, Max>::runOnRegion(
    Grid *const grid, InnieOutieRegion &region,
    std::vector<InnieOutieRegion *> &to_remove) {
  {
    Cage pseudo_cage;
    for (auto &io : region.innies_outies) {
      for (auto *c : io.inside_cage)
        pseudo_cage.cells.push_back(c);
    }
    if (!pseudo_cage.empty()) {
      if (region.known_cage.sum >= region.expected_sum)
        throw invalid_grid_exception{"invalid set of innies"};
      if (pseudo_cage.size() >= Min && pseudo_cage.size() <= Max) {
        pseudo_cage.is_pseudo = true;
        pseudo_cage.pseudo_name = region.getName() + " innies";
        pseudo_cage.sum = region.expected_sum - region.known_cage.sum;
        Cage *the_cage = getOrCreatePseudoCage(
            grid, region, region.large_innies, pseudo_cage);
        if (reduceCombinations(region, *the_cage, the_cage->sum, "innie",
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
      for (auto *c : io.outside_cage)
        pseudo_cage.cells.push_back(c);
    }
    if (!pseudo_cage.empty()) {
      if (region.known_cage.sum + outie_cage_sum <= region.expected_sum)
        throw invalid_grid_exception{"invalid set of outies"};
      if (pseudo_cage.size() >= Min && pseudo_cage.size() <= Max) {
        pseudo_cage.is_pseudo = true;
        pseudo_cage.pseudo_name = region.getName() + " outies";
        pseudo_cage.sum =
            region.known_cage.sum + outie_cage_sum - region.expected_sum;
        Cage *the_cage = getOrCreatePseudoCage(
            grid, region, region.large_outies, pseudo_cage);
        if (reduceCombinations(region, *the_cage, the_cage->sum, "outie",
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
