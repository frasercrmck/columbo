#include "fixed_cell_cleanup.h"

StepCode PropagateFixedCells::runOnHouse(House &house, const Cell *fixed_cell) {
  bool modified = false;
  const Mask fixed_mask = fixed_cell->candidates.to_ulong();

  for (auto &c : house) {
    // Not interested in fixed cells
    if (c == fixed_cell || c->isFixed()) {
      continue;
    }

    CandidateSet *candidates = &c->candidates;
    const Mask intersection = candidates->to_ulong() & fixed_mask;

    // Nothing in this cell would be changed
    if (!intersection) {
      continue;
    }

    if (DEBUG) {
      dbgs() << "Clean Up: removing " << printCandidateString(intersection)
             << " from " << c->coord << "\n";
    }

    modified = true;
    changed.insert(c);
    work_list.insert(c);
    *candidates = CandidateSet(candidates->to_ulong() & ~fixed_mask);
  }

  return {false, modified};
}
