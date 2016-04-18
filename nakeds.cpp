#include "nakeds.h"
#include "debug.h"

#include <set>

StepCode EliminateNakedPairsStep::runOnHouse(House &cell_list) {
  bool modified = false;
  std::set<Mask> found_masks;
  std::set<Mask> duplicate_masks;
  for (const Cell *cell : cell_list) {
    if (cell->candidates.none()) {
      return {true, modified};
    }
    if (cell->candidates.count() != 2) {
      continue;
    }

    const Mask mask = cell->candidates.to_ulong();

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
      if (candidates->to_ulong() == mask) {
        continue;
      }

      auto new_cands = CandidateSet(candidates->to_ulong() & ~mask);
      if (*candidates == new_cands) {
        continue;
      }

      if (DEBUG) {
        const Mask intersection = candidates->to_ulong() & mask;
        dbgs() << "Naked Pair " << printCandidateString(mask) << " removes "
               << printCandidateString(intersection) << " from " << cell->coord
               << "\n";
      }

      modified = true;
      changed.insert(cell);
      *candidates = new_cands;
    }
  }

  return {false, modified};
}

StepCode EliminateNakedTriplesStep::runOnHouse(House &house) {
  bool modified = false;

  if (house.size() <= 3) {
    return {false, modified};
  }

  std::vector<std::pair<Mask, std::vector<const Cell *>>> found_masks;
  for (const Cell *cell : house) {
    std::size_t num_candidates = cell->candidates.count();
    if (num_candidates == 0) {
      return {true, modified};
    }
    if (num_candidates != 2 && num_candidates != 3) {
      continue;
    }

    const Mask mask = cell->candidates.to_ulong();

    bool found_match = false;
    for (unsigned i = 0; i < found_masks.size(); ++i) {
      const Mask m = found_masks[i].first;

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
      std::pair<Mask, std::vector<const Cell *>> pair;
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
    const Mask mask = pair.first;

    for (Cell *cell : house) {
      if (cell->isFixed()) {
        continue;
      }

      if (std::find(matches.begin(), matches.end(), cell) != matches.end()) {
        continue;
      }

      CandidateSet *candidates = &cell->candidates;
      if (candidates->to_ulong() == mask) {
        continue;
      }
      auto new_cands = CandidateSet(candidates->to_ulong() & ~mask);
      if (*candidates == new_cands) {
        continue;
      }

      if (DEBUG) {
        const Mask intersection = candidates->to_ulong() & mask;
        dbgs() << "Naked Triple " << printCandidateString(mask) << " removes "
               << printCandidateString(intersection) << " from " << cell->coord
               << "\n";
      }

      modified = true;
      changed.insert(cell);
      *candidates = new_cands;
    }
  }

  return {false, modified};
}
