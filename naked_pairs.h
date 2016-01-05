#ifndef SOLVER_NAKED_PAIRS_H
#define SOLVER_NAKED_PAIRS_H

#include "defs.h"
#include "debug.h"

#include <set>

template <typename T> static bool eliminateNakedPairs(T &cell_list) {
  bool modified = false;
  std::set<unsigned long> found_masks;
  std::set<unsigned long> duplicate_masks;
  for (const Cell *cell : cell_list) {
    if (cell->candidates.count() != 2) {
      continue;
    }

    const unsigned long mask = cell->candidates.to_ulong();

    if (found_masks.count(mask)) {
      duplicate_masks.insert(cell->candidates.to_ulong());
    }

    found_masks.insert(mask);
  }

  for (auto &mask : duplicate_masks) {
    for (Cell *cell : cell_list) {
      if (cell->isFixed()) {
        continue;
      }
      CandidateSet *candidates = &cell->candidates;
      if (candidates->to_ulong() != mask) {
        auto new_cands = CandidateSet(candidates->to_ulong() & ~mask);
        if (*candidates != new_cands) {
          modified = true;
          if (DEBUG) {
            const unsigned long intersection = candidates->to_ulong() & mask;
            dbgs() << "Naked Pair " << printCandidateString(mask) << " removes "
                   << printCandidateString(intersection) << " from "
                   << cell->coord << "\n";
          }
        }
        *candidates = new_cands;
      }
    }
  }

  return modified;
}

static bool eliminateNakedPairs(HouseArray &rows, HouseArray &cols,
                                HouseArray &boxes, CageList &cages) {
  bool modified = false;
  for (auto &row : rows) {
    modified |= eliminateNakedPairs(row);
  }
  for (auto &col : cols) {
    modified |= eliminateNakedPairs(col);
  }
  for (auto &box : boxes) {
    modified |= eliminateNakedPairs(box);
  }
  for (auto &cage : cages) {
    modified |= eliminateNakedPairs(cage);
  }
  return modified;
}

#endif // SOLVER_NAKED_PAIRS_H
