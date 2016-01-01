#ifndef SOLVER_KILLER_COMBOS_H
#define SOLVER_KILLER_COMBOS_H

#include "defs.h"
#include "utils.h"

void eliminateKillerCombos(std::vector<Cage> &cages) {
  for (auto &cage : cages) {
    std::vector<std::vector<int>> subsets;
    const unsigned subset_size = static_cast<unsigned>(cage.cells.size());
    generateDefaultFixedSizeSubsets(cage.sum, subset_size, subsets);

    if (subsets.size() != 1) {
      continue;
    }

    for (auto &cell : cage.cells) {
      unsigned long mask = 0u;
      for (auto &i : subsets[0]) {
        mask |= (1 << (i - 1));
      }

      cell->candidates = CandidateSet(mask);
    }
  }
}

bool eliminateImpossibleCombos(std::vector<Cage> &cages) {
  bool modified = false;
  for (auto &cage : cages) {
    unsigned long mask = 0u;
    for (auto &cell : cage.cells) {
      mask |= cell->candidates.to_ulong();
    }

    std::vector<int> possibles;
    for (int i = 0; i < 9; ++i) {
      if ((mask >> i) & 0x1) {
        possibles.push_back(i + 1);
      }
    }

    std::vector<std::vector<int>> subsets;
    generateFixedSizeSubsets(cage.sum, static_cast<unsigned>(cage.cells.size()),
                             possibles, subsets);

    unsigned long possibles_mask = 0u;
    for (auto subset : subsets) {
      for (auto val : subset) {
        possibles_mask |= (1 << (val - 1));
      }
    }

    for (auto &cell : cage.cells) {
      CandidateSet *candidates = &cell->candidates;
      auto new_cands = CandidateSet(candidates->to_ulong() & possibles_mask);
      modified |= *candidates != new_cands;
      *candidates = new_cands;
    }
  }

  return modified;
}

#endif // SOLVER_KILLER_COMBOS_H
