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

template <unsigned Size>
bool EliminateNakedsStep<Size>::runOnHouse(House &house) {
  bool modified = false;

  if (house.size() <= Size) {
    return modified;
  }

  // TODO: Use a map?
  // TODO: Abstract retrieval of nakeds into utility
  std::vector<std::pair<Mask, std::vector<const Cell *>>> found_masks;

  for (const Cell *cell : house) {
    std::size_t num_candidates = cell->candidates.count();
    if (num_candidates == 0) {
      throw invalid_grid_exception{};
    }
    if (num_candidates == 1 || num_candidates > Size) {
      continue;
    }

    const Mask candidate_mask = cell->candidates.to_ulong();

    bool found_match = false;
    for (auto & [ m, matches ] : found_masks) {
      // ORing the two masks together will switch on all candidates found in
      // both masks. If it's less than three, then we're still a triple
      // candidate.
      // TODO: If the current mask is size 2 and the OR of the two is 3, then
      // we should create two masks: one for the two and one for the OR.
      // Otherwise you could get 1/2 match with 1/2/3 and 1/2/4.
      // For now, only accept found masks of size 3. Easy naked triples.
      if (m.count() == Size && (candidate_mask | m).count() == Size) {
        found_match = true;
        matches.push_back(cell);
      }
    }

    if (!found_match) {
      std::pair<Mask, std::vector<const Cell *>> pair;
      pair.first = candidate_mask;
      pair.second.push_back(cell);
      found_masks.push_back(pair);
    }
  }

  for (const auto & [ mask, cells ] : found_masks) {
    if (cells.size() != Size) {
      continue;
    }

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
