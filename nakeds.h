#ifndef SOLVER_NAKEDS_H
#define SOLVER_NAKEDS_H

#include "defs.h"
#include "debug.h"

#include <set>

static bool eliminateNakedPairs(House &cell_list) {
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
                                HouseArray &boxes) {
  bool modified = false;
  for (auto &row : rows) {
    modified |= eliminateNakedPairs(*row);
  }
  for (auto &col : cols) {
    modified |= eliminateNakedPairs(*col);
  }
  for (auto &box : boxes) {
    modified |= eliminateNakedPairs(*box);
  }
  return modified;
}

static bool eliminateNakedTriples(House &house) {
  bool modified = false;

  if (house.size() <= 3) {
    return modified;
  }

  std::vector<std::pair<unsigned long, std::vector<const Cell *>>> found_masks;
  for (const Cell *cell : house) {
    std::size_t num_candidates = cell->candidates.count();
    if (num_candidates != 2 && num_candidates != 3) {
      continue;
    }

    const unsigned long mask = cell->candidates.to_ulong();

    bool found_match = false;
    for (unsigned i = 0; i < found_masks.size(); ++i) {
      const unsigned long m = found_masks[i].first;

      // ORing the two masks together will switch on all candidates found in
      // both masks. If it's less than three, then we're still a triple
      // candidate.
      // TODO: If the current mask is size 2 and the OR of the two is 3, then
      // we should create two masks: one for the two and one for the OR.
      // Otherwise you could get 1/2 match with 1/2/3 and 1/2/4.
      // For now, only accept found masks of size 3. Easy naked triples.
      if (bitCount(m) == 3 && bitCount(mask | m) == 3) {
        found_match = true;
        found_masks[i].second.push_back(cell);
      }
    }

    if (!found_match) {
      std::pair<unsigned long, std::vector<const Cell *>> pair;
      pair.first = mask;
      pair.second.push_back(cell);
      found_masks.push_back(pair);
    }
  }

  for (auto &pair : found_masks) {
    if (pair.second.size() != 3) {
      continue;
    }

    auto &matches = pair.second;
    const unsigned long mask = pair.first;

    for (Cell *cell : house) {
      if (cell->isFixed()) {
        continue;
      }

      if (std::find(matches.begin(), matches.end(), cell) != matches.end()) {
        continue;
      }

      CandidateSet *candidates = &cell->candidates;
      if (candidates->to_ulong() != mask) {
        auto new_cands = CandidateSet(candidates->to_ulong() & ~mask);
        if (*candidates != new_cands) {
          modified = true;
          if (DEBUG) {
            const unsigned long intersection = candidates->to_ulong() & mask;
            dbgs() << "Naked Triple " << printCandidateString(mask)
                   << " removes " << printCandidateString(intersection)
                   << " from " << cell->coord << "\n";
          }
        }
        *candidates = new_cands;
      }
    }
  }

  return modified;
}

static bool eliminateNakedTriples(HouseArray &rows, HouseArray &cols,
                                  HouseArray &boxes) {
  bool modified = false;
  for (auto &row : rows) {
    modified |= eliminateNakedTriples(*row);
  }
  for (auto &col : cols) {
    modified |= eliminateNakedTriples(*col);
  }
  for (auto &box : boxes) {
    modified |= eliminateNakedTriples(*box);
  }
  return modified;
}

#endif // SOLVER_NAKEDS_H