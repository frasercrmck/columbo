#ifndef COLUMBO_NAKEDS_H
#define COLUMBO_NAKEDS_H

#include "debug.h"
#include "defs.h"
#include "step.h"
#include <algorithm>

template <unsigned Size> struct EliminateNakedsStep : ColumboStep {
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

  virtual void anchor() override = 0;

  const char *getID() const override = 0;
  const char *getName() const override = 0;

private:
  bool runOnHouse(House &house, bool debug);
};

struct EliminateNakedPairsStep : public EliminateNakedsStep<2> {
  virtual void anchor() override;
  const char *getID() const override { return "naked-pairs"; }
  const char *getName() const override { return "Naked Pairs"; }
};

struct EliminateNakedTriplesStep : public EliminateNakedsStep<3> {
  virtual void anchor() override;
  const char *getID() const override { return "naked-triples"; }
  const char *getName() const override { return "Naked Triples"; }
};

struct EliminateNakedQuadsStep : public EliminateNakedsStep<4> {
  virtual void anchor() override;
  const char *getID() const override { return "naked-quads"; }
  const char *getName() const override { return "Naked Quads"; }
};

struct EliminateNakedQuintsStep : public EliminateNakedsStep<5> {
  virtual void anchor() override;
  const char *getID() const override { return "naked-quints"; }
  const char *getName() const override { return "Naked Quints"; }
};

template <unsigned Size> struct Naked {
  Mask mask;
  std::array<CellCageUnit, Size> units;

  const char *getName() const {
    static_assert(Size >= 2 && Size <= 5 && "Invalid size");
    if constexpr (Size == 2) {
      return "Naked Pair";
    } else if constexpr (Size == 3) {
      return "Naked Triple";
    } else if constexpr (Size == 4) {
      return "Naked Quad";
    } else {
      return "Naked Quint";
    }
  }
};

template <unsigned Size> std::vector<Naked<Size>> getNakeds(House &house) {
  using MaskCellsPair = std::pair<Mask, std::vector<CellCageUnit>>;
  std::vector<Naked<Size>> nakeds;
  std::vector<MaskCellsPair> potential_nakeds;

  for (Cell *cell : house) {
    std::size_t num_candidates = cell->candidates.count();
    if (num_candidates == 0) {
      throw invalid_grid_exception{};
    } else if (num_candidates == 1 || num_candidates > Size) {
      continue;
    }

    const Mask candidate_mask = cell->candidates;

    // Fold this cell in with previous ones
    std::for_each(potential_nakeds.begin(), potential_nakeds.end(),
                  [cell, candidate_mask](MaskCellsPair &p) {
                    if ((p.first | candidate_mask) == p.first)
                      p.second.push_back(cell);
                  });

    // Create new combinations, e.g. {12} and {23} can merge to become {123}.
    std::vector<MaskCellsPair> new_candidates;
    for (auto &[a, b] : potential_nakeds) {
      if (a.count() <= candidate_mask.count() && a.count() <= Size &&
          ((a | candidate_mask) == candidate_mask)) {
        // Create a new entry for this one, in case future cells match it
        new_candidates.push_back(MaskCellsPair{candidate_mask, {cell}});
        for (auto &unit : b) {
          if (!unit.overlapsWith(cell))
            new_candidates.back().second.push_back(unit);
        }
      }
    }
    potential_nakeds.insert(std::end(potential_nakeds),
                            std::begin(new_candidates),
                            std::end(new_candidates));

    // Create a new entry for this one, in case future cells match it
    potential_nakeds.push_back(MaskCellsPair{candidate_mask, {cell}});
  }

  std::unordered_set<Cage *> visited;
  for (Cell *cell : house) {
    for (Cage *cage : cell->all_cages()) {
      if (!visited.insert(cage).second || !cage->cage_combos)
        continue;
      if (!cage->areAllCellsAlignedWith(house))
        continue;
      for (auto mask : cage->cage_combos->computeKillerPairs()) {
        std::size_t num_candidates = mask.count();
        if (num_candidates == 0) {
          throw invalid_grid_exception{};
        } else if (num_candidates == 1 || num_candidates > Size) {
          continue;
        }

        // Fold this cell in with previous ones
        std::for_each(potential_nakeds.begin(), potential_nakeds.end(),
                      [cell, cage, mask](MaskCellsPair &p) {
                        for (auto &unit : p.second) {
                          if (unit.overlapsWith(cage) ||
                              unit.overlapsWith(cell))
                            return;
                        }
                        if ((p.first | mask).count() <= Size) {
                          p.second.push_back(cage);
                        }
                      });
        // Create a new entry for this one, in case future cells match it
        potential_nakeds.push_back(MaskCellsPair{mask, {cage}});
      }
    }
  }

  // Now filter them
  for (auto &[mask, units] : potential_nakeds) {
    // The only "true" nakeds are those with Size candidates and Size cells
    if (mask.count() == Size && units.size() == Size) {
      Naked<Size> naked{mask, {{}}};
      std::copy(units.begin(), units.end(), naked.units.begin());
      nakeds.emplace_back(std::move(naked));
    }
  }

  return nakeds;
}

template <unsigned Size>
bool EliminateNakedsStep<Size>::runOnHouse(House &house, bool debug) {
  bool modified = false;
  if (house.size() <= Size) {
    return modified;
  }

  for (const auto &naked : getNakeds<Size>(house)) {
    bool printed = false;
    const auto &[mask, units] = naked;
    for (Cell *cell : house) {
      if (cell->isFixed()) {
        continue;
      }

      if (std::find(units.begin(), units.end(), cell) != units.end()) {
        continue;
      }
      if (std::any_of(std::begin(units), std::end(units),
                      [cell](CellCageUnit const &unit) {
                        return unit.is_or_contains(cell);
                      }))
        continue;

      if (auto intersection = updateCell(cell, ~mask)) {
        modified = true;
        if (debug) {
          if (!printed) {
            printed = true;
            dbgs() << getName() << " " << printCandidateString(mask) << " ("
                   << house.getPrintKind() << " " << getHousePrintNum(house)
                   << ") [";
            for (unsigned i = 0, e = units.size(); i != e; i++) {
              CellCageUnit const &unit = units[i];
              unit.printCellList(dbgs());
              if (unit.cage)
                dbgs() << '(' << *unit.cage << ')';
              if (i < e - 1)
                dbgs() << "/";
            }
            dbgs() << "]:\n";
          }
          dbgs() << "\tRemoving " << printCandidateString(*intersection)
                 << " from " << cell->coord << "\n";
        }
      }
    }
  }

  return modified;
}

#endif // COLUMBO_NAKEDS_H
