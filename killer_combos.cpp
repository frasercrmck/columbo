#include "killer_combos.h"

#include "debug.h"
#include "utils.h"

bool EliminateImpossibleCombosStep::runOnCage(Cage &cage) {
  bool modified = false;

  std::vector<IntList> &subsets = (*cage_combo_map)[&cage];

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

    auto new_cands = cell->candidates & possibles_mask;

    if (new_cands.none()) {
      throw invalid_grid_exception{};
    }

    if (cell->candidates == new_cands) {
      continue;
    }

    if (DEBUG) {
      dbgs() << "Killer Combos: setting " << cell->coord << " to "
             << printCandidateString(new_cands.to_ulong()) << "\n";
    }

    modified = true;
    cell->candidates = new_cands;
    changed.insert(cell);
  }

  return modified;
}
