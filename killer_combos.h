#ifndef COLUMBO_KILLER_COMBOS_H
#define COLUMBO_KILLER_COMBOS_H

#include "defs.h"
#include "utils.h"
#include "debug.h"

bool eliminateImpossibleCombos(Cage &cage) {
  bool modified = false;

  std::vector<IntList> possibles;
  possibles.resize(cage.cells.size());

  unsigned idx = 0;
  for (auto &cell : cage.cells) {
    for (unsigned x = 0; x < 9; ++x) {
      if (cell->candidates.test(static_cast<std::size_t>(x))) {
        possibles[idx].push_back(x + 1);
      }
    }
    ++idx;
  }

  std::vector<IntList> subsets;
  generateSubsetSums(cage.sum, possibles, subsets);

  // For each cage, check all resulting subsets for new possible values
  // Say we return [1, 8], [2, 7], [7, 2] as all possible values for two cells.
  // Then we want the set the first cell's candidates to {1/2/7} and the
  // second's to {2/7/8}
  for (unsigned i = 0; i < cage.cells.size(); ++i) {
    Cell *cell = cage.cells[i];
    Mask possibles_mask = 0u;
    for (auto subset : subsets) {
      possibles_mask |= (1 << (subset[i] - 1));
    }
    CandidateSet *candidates = &cell->candidates;
    auto new_cands = CandidateSet(candidates->to_ulong() & possibles_mask);
    if (*candidates != new_cands) {
      modified = true;
      if (DEBUG) {
        dbgs() << "Killer Combos: setting " << cell->coord << " to "
               << printCandidateString(new_cands.to_ulong()) << "\n";
      }
    }
    *candidates = new_cands;
  }

  return modified;
}

bool eliminateImpossibleCombos(Grid *const grid) {
  bool modified = false;
  for (auto &cage : grid->cages) {
    modified |= eliminateImpossibleCombos(cage);
  }

  return modified;
}

#endif // COLUMBO_KILLER_COMBOS_H
