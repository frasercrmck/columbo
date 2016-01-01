#ifndef SOLVER_SINGLES_H
#define SOLVER_SINGLES_H

#include "defs.h"

static bool eliminateSingles(House &house) {
  bool modified = false;
  std::vector<unsigned long> single_masks;
  for (auto &cell : house) {
    if (cell->isFixed()) {
      single_masks.push_back(cell->candidates.to_ulong());
    }
  }

  for (auto &single_mask : single_masks) {
    for (auto &cell : house) {
      CandidateSet *candidates = &cell->candidates;
      if (candidates->to_ulong() != single_mask) {
        auto new_cands = CandidateSet(candidates->to_ulong() & ~single_mask);
        modified |= *candidates != new_cands;
        *candidates = new_cands;
      }
    }
  }

  return modified;
}

static bool eliminateSingles(HouseArray &rows, HouseArray &cols,
                             HouseArray &boxes) {
  bool modified = false;
  for (auto &row : rows) {
    modified |= eliminateSingles(row);
  }
  for (auto &col : cols) {
    modified |= eliminateSingles(col);
  }
  for (auto &box : boxes) {
    modified |= eliminateSingles(box);
  }
  return modified;
}

#endif // SOLVER_SINGLES_H
