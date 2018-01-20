#ifndef COLUMBO_NAKEDS_H
#define COLUMBO_NAKEDS_H

#include "debug.h"
#include "defs.h"
#include "step.h"

template <unsigned Size> struct EliminateNakedsStep : ColumboStep {
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

  virtual void anchor() override = 0;

  const char *getID() const override = 0;
  const char *getName() const override = 0;

private:
  bool runOnHouse(House &house);
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

template <unsigned Size> struct Naked {
  Mask mask;
  std::array<const Cell *, Size> cells;

  const char *getName() const {
    static_assert(Size >= 2 && Size <= 4 && "Invalid size");
    if constexpr (Size == 2) {
      return "Naked Pair";
    } else if constexpr (Size == 3) {
      return "Naked Triple";
    } else {
      return "Naked Quad";
    }
  }
};

template <unsigned Size>
std::vector<Naked<Size>> getNakeds(const House &house) {
  using MaskCellsPair = std::pair<Mask, std::vector<const Cell *>>;
  std::vector<Naked<Size>> nakeds;
  std::vector<MaskCellsPair> potential_nakeds;

  for (const Cell *cell : house) {
    std::size_t num_candidates = cell->candidates.count();
    if (num_candidates == 0) {
      throw invalid_grid_exception{};
    } else if (num_candidates == 1 || num_candidates > Size) {
      continue;
    }

    const Mask candidate_mask = cell->candidates.to_ulong();

    // Fold this cell in with previous ones
    std::for_each(potential_nakeds.begin(), potential_nakeds.end(),
                  [cell, candidate_mask](MaskCellsPair &p) {
                    if ((p.first | candidate_mask).count() <= Size) {
                      p.second.push_back(cell);
                    }
                  });
    // Create a new entry for this one, in case future cells match it
    potential_nakeds.push_back(MaskCellsPair{candidate_mask, {cell}});
  }

  // Now filter them
  for (auto & [ mask, cells ] : potential_nakeds) {
    // The only "true" nakeds are those with Size candidates and Size cells
    if (mask.count() == Size && cells.size() == Size) {
      Naked<Size> naked{mask, {{}}};
      std::copy(cells.begin(), cells.end(), naked.cells.begin());
      nakeds.emplace_back(std::move(naked));
    }
  }

  return nakeds;
}

template <unsigned Size>
bool EliminateNakedsStep<Size>::runOnHouse(House &house) {
  bool modified = false;
  if (house.size() <= Size) {
    return modified;
  }

  for (const auto &naked : getNakeds<Size>(house)) {
    const auto & [ mask, cells ] = naked;
    for (Cell *cell : house) {
      if (cell->isFixed()) {
        continue;
      }

      if (std::find(cells.begin(), cells.end(), cell) != cells.end()) {
        continue;
      }

      if (auto intersection = updateCell(cell, ~mask)) {
        modified = true;
        if (DEBUG) {
          dbgs() << getName() << " " << printCandidateString(mask)
                 << " removes " << printCandidateString(*intersection)
                 << " from " << cell->coord << "\n";
        }
      }
    }
  }

  return modified;
}

#endif // COLUMBO_NAKEDS_H
