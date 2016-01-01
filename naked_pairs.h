#ifndef SOLVER_NAKED_PAIRS_H
#define SOLVER_NAKED_PAIRS_H

#include "defs.h"

#include <set>

static bool eliminateNakedPairs(House &house) {
  bool modified = false;
  std::set<unsigned long> found_masks;
  std::set<unsigned long> duplicate_masks;
  for (auto &cell : house) {
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
    for (auto &cell : house) {
      CandidateSet *candidates = &cell->candidates;
      if (candidates->to_ulong() != mask) {
        auto new_cands = CandidateSet(candidates->to_ulong() & ~mask);
        modified |= *candidates != new_cands;
        *candidates = new_cands;
      }
    }
  }

  return modified;
}

static bool eliminateNakedPairs(HouseArray &rows, HouseArray &cols,
                                HouseArray &boxes) {
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
  return modified;
}

#endif // SOLVER_NAKED_PAIRS_H
