#include "killer_combos.h"

#include "debug.h"
#include "utils.h"

StepCode EliminateImpossibleCombosStep::runOnCage(Cage &cage) {
  bool modified = false;

  std::vector<IntList> &subsets = (*map)[&cage];

  // For each cage, check all resulting subsets for new possible values
  // Say we return [1, 8], [2, 7], [7, 2] as all possible values for two cells.
  // Then we want the set the first cell's candidates to {1/2/7} and the
  // second's to {2/7/8}
  for (unsigned i = 0; i < cage.cells.size(); ++i) {
    Cell *cell = cage.cells[i];
    Mask possibles_mask = 0u;
    for (auto &subset : subsets) {
      possibles_mask |= (1 << (subset[i] - 1));
    }
    CandidateSet *candidates = &cell->candidates;
    auto new_cands = CandidateSet(candidates->to_ulong() & possibles_mask);
    if (*candidates != new_cands) {
      modified = true;
      changed.insert(cell);
      if (DEBUG) {
        dbgs() << "Killer Combos: setting " << cell->coord << " to "
               << printCandidateString(new_cands.to_ulong()) << "\n";
      }
    }
    *candidates = new_cands;
    if (new_cands.none()) {
      return {true, modified};
    }
  }

  return {false, modified};
}

