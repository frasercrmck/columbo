#ifndef SOLVER_KILLER_COMBOS_H
#define SOLVER_KILLER_COMBOS_H

#include "defs.h"
#include "utils.h"

bool eliminateImpossibleCombos(Cage &cage) {
  bool modified = false;
  unsigned long mask = 0u;
  for (auto &cell : cage.cells) {
    mask |= cell->candidates.to_ulong();
  }

  IntList possibles;
  for (int i = 0; i < 9; ++i) {
    if ((mask >> i) & 0x1) {
      possibles.push_back(i + 1);
    }
  }

  std::vector<IntList> subsets;
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
  return modified;
}

bool eliminateImpossibleCombos(CageList &cages) {
  bool modified = false;
  for (auto &cage : cages) {
    modified |= eliminateImpossibleCombos(cage);
  }

  return modified;
}

#endif // SOLVER_KILLER_COMBOS_H
