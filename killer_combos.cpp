#include "killer_combos.h"

#include "debug.h"
#include "utils.h"
#include <unordered_set>
#include <algorithm>

bool EliminateImpossibleCombosStep::runOnCage(Cage &cage) {
  bool modified = false;

  if (!cage.cage_combos)
    throw invalid_grid_exception{"Cages must have combo information"};

  auto &cage_combos = *cage.cage_combos;

  // For each cage, check all resulting cage combos for new possible values
  // Say we return [1, 8], [2, 7], [7, 2] as all possible values for two cells.
  // Then we want the set the first cell's candidates to {1/2/7} and the
  // second's to {2/7/8}
  for (unsigned i = 0; i < cage.cells.size(); ++i) {
    Cell *cell = cage.cells[i];
    Mask possibles_mask = 0u;
    for (auto const &cage_combo : cage_combos)
      for (auto const &perm : cage_combo.permutations)
        possibles_mask |= (1 << (perm[i] - 1));

    auto new_cands = cell->candidates & possibles_mask;

    if (new_cands.none()) {
      throw invalid_grid_exception{};
    }

    if (cell->candidates == new_cands) {
      continue;
    }

    if (DEBUG) {
      dbgs() << "\t" << getKind() << ": setting " << cell->coord << " to "
             << printCandidateString(new_cands.to_ulong()) << "\n";
    }

    modified = true;
    cell->candidates = new_cands;
    changed.insert(cell);
  }

  return modified;
}

bool EliminateConflictingCombosStep::runOnHouse(House &house) {
  std::unordered_set<const Cage *> visited;
  std::unordered_set<const Cell *> members{house.begin(), house.end()};

  for (auto *cell : house.cells) {
    auto *cage = cell->cage;
    if ((cage->size() != 2 && cage->size() != 3) ||
        !visited.insert(cage).second)
      continue;
    if (!cage->areAllCellsAlignedWith(house))
      continue;

    std::unordered_set<const Cage *> other_visited;
    std::unordered_map<Mask, CellCageUnit> invalid_subsets;

    if (!cage->cage_combos)
      throw invalid_grid_exception{"Cages must have combo information"};

    auto &cage_combos = *cage->cage_combos;

    std::unordered_set<Mask> unique_combos =
        cage_combos.getUniqueCombinations();

    for (auto *other_cell : house.cells) {
      if (other_cell == cell || other_cell->cage == cage ||
          !other_visited.insert(other_cell->cage).second)
        continue;

      // Check if this combination would invalidate all candidates for this
      // particular cell.
      if (std::all_of(
              std::begin(cage->cells), std::end(cage->cells),
              [other_cell](const Cell *c) { return c->canSee(other_cell); }))
        for (auto const &combo_mask : unique_combos)
          if (combo_mask == other_cell->candidates)
            invalid_subsets[combo_mask] = CellCageUnit{other_cell};

      // Else, try and determine a set of combinations that this other cell's
      // cage must have. For instance, we know that 14/2 must be {48|59} so
      // must include at least one of {48|49|58|59}. This would conflict with a
      // potential combination for {48} for cage 12/2, rendering it impossible.
      if (invalid_subsets.empty()) {
        if (!other_cell->cage->areAllCellsAlignedWith(house))
          continue;

        for (auto m : other_cell->cage->cage_combos->computeKillerPairs()) {
          if (unique_combos.count(m))
            invalid_subsets[m] = CellCageUnit{other_cell->cage};
        }
      }
    }

    if (invalid_subsets.empty())
      continue;

    for (auto [invalid, conflict] : invalid_subsets) {
      if (DEBUG) {
        dbgs() << "Conflicting Combos: cage " << *cage << " combination "
               << printCandidateString(invalid) << " conflicts with "
               << conflict.getName() << " " << conflict
               << " whose candidates must include at least one of "
               << printCandidateString(invalid) << ":\n";
      }
      cage_combos.combos.erase(
          std::remove_if(std::begin(cage_combos), std::end(cage_combos),
                         [inv = invalid](CageCombo const &combo) {
                           return combo.combo == inv;
                         }),
          std::end(cage_combos));
      // Try and remove candidates from this cage's cells.
      if (runOnCage(*cage))
        return true;
    }
  }

  return false;
}
