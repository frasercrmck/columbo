#include "killer_combos.h"

#include "debug.h"
#include "utils.h"
#include <unordered_set>
#include <algorithm>

bool EliminateImpossibleCombosStep::runOnCage(Cage &cage, bool debug,
                                              std::string_view dbg_reason) {
  bool modified = false;

  if (!cage.cage_combos)
    throw invalid_grid_exception{"Cages must have combo information"};

  auto &cage_combos = *cage.cage_combos;

  // For each cage, check all resulting cage combos for new possible values
  // Say we return [1, 8], [2, 7], [7, 2] as all possible values for two cells.
  // Then we want the set the first cell's candidates to {1/2/7} and the
  // second's to {2/7/8}
  bool printed = false;
  for (unsigned i = 0; i < cage.cells.size(); ++i) {
    Cell *cell = cage.cells[i];
    Mask possibles_mask = 0u;
    for (auto const &cage_combo : cage_combos)
      for (auto const &perm : cage_combo.permutations)
        possibles_mask |= (1 << (perm[i] - 1));

    auto new_cands = cell->candidates & possibles_mask;

    if (possibles_mask.none()) {
      std::stringstream ss;
      ss << "Cell " << cell->coord << " of cage " << cage
         << " has no possible combinations left";
      throw invalid_grid_exception{ss.str()};
    }
    if (new_cands.none()) {
      std::stringstream ss;
      ss << "Cell " << cell->coord << " candidates "
         << printCandidateString(cell->candidates)
         << " would be cancelled out by removing "
         << printCandidateString(possibles_mask);
      throw invalid_grid_exception{ss.str()};
    }

    if (cell->candidates == new_cands) {
      continue;
    }

    if (debug) {
      if (!printed && !dbg_reason.empty()) {
        printed = true;
        dbgs() << dbg_reason;
      }
      dbgs() << "\t" << getKind() << ": setting " << cell->coord << " to "
             << printCandidateString(new_cands) << "\n";
    }

    modified = true;
    cell->candidates = new_cands;
    changed.insert(cell);
  }

  return modified;
}

bool EliminateConflictingCombosStep::runOnHouse(House &house, bool debug) {
  std::unordered_set<const Cage *> visited;

  std::vector<Cage *> cage_list;
  for (auto *cell : house.cells) {
    for (auto *pcage : cell->all_cages()) {
      if (visited.insert(pcage).second &&
          pcage->areAllCellsAlignedWith(house)) {
        cage_list.push_back(pcage);
      }
    }
  }

  for (auto *cage : cage_list) {
    std::unordered_set<const Cage *> other_visited;
    std::unordered_map<Mask, std::pair<Mask, CellCageUnit>> invalid_subsets;

    if (!cage->cage_combos)
      throw invalid_grid_exception{"Cages must have combo information"};

    auto &cage_combos = *cage->cage_combos;

    std::unordered_set<Mask> unique_combos =
        cage_combos.getUniqueCombinations();

    for (auto *other_cell : house.cells) {
      if (cage->contains(other_cell))
        continue;

      // Check if this combination would invalidate all candidates for this
      // particular cell.
      if (std::all_of(
              std::begin(cage->cells), std::end(cage->cells),
              [other_cell](const Cell *c) { return c->canSee(other_cell); }))
        for (auto const &combo_mask : unique_combos)
          if (combo_mask == other_cell->candidates)
            invalid_subsets[combo_mask] = {combo_mask,
                                           CellCageUnit{other_cell}};

      // Else, try and determine a set of combinations that this other cell's
      // cage must have. For instance, we know that 14/2 must be {48|59} so
      // must include at least one of {48|49|58|59}. This would conflict with a
      // potential combination for {48} for cage 12/2, rendering it impossible.
      if (invalid_subsets.empty()) {
        for (auto *other_cage : other_cell->all_cages()) {
          if (!other_cage->cage_combos ||
              !other_cage->areAllCellsAlignedWith(house) ||
              other_cage->overlapsWith(cage))
            continue;
          for (auto m : other_cage->cage_combos->computeKillerPairs(
                   static_cast<unsigned>(cage->size()))) {
            for (auto &unique : unique_combos)
              if ((unique & m) == m)
                invalid_subsets[unique] = {m, CellCageUnit{other_cage}};
          }
        }
      }
    }

    if (invalid_subsets.empty())
      continue;

    for (auto &[invalid, conflict_data] : invalid_subsets) {
      std::stringstream ss;
      if (debug) {
        const auto &[conflict_mask, conflict_unit] = conflict_data;
        ss << "Conflicting Combos: cage " << *cage << " combination "
           << printCandidateString(invalid) << " conflicts with "
           << conflict_unit.getName() << " " << conflict_unit
           << " whose candidates must include at least one of "
           << printCandidateString(conflict_mask) << ":\n";
      }
      cage_combos.combos.erase(
          std::remove_if(std::begin(cage_combos), std::end(cage_combos),
                         [inv = invalid](CageCombo const &combo) {
                           return combo.combo == inv;
                         }),
          std::end(cage_combos));
      // Try and remove candidates from this cage's cells.
      if (runOnCage(*cage, debug, ss.str()))
        return true;
    }
  }

  return false;
}
