#include "killer_combos.h"

#include "debug.h"
#include "utils.h"
#include "combinations.h"
#include <unordered_set>
#include <algorithm>

bool EliminateImpossibleCombosStep::runOnCage(Cage &cage, bool debug,
                                              std::string_view dbg_reason) {
  bool modified = false;

  if (!cage.cage_combos)
    throw invalid_grid_exception{"Cages must have combo information"};
  // For each cage, check all resulting cage combos for new possible values
  // Say we return [1, 8], [2, 7], [7, 2] as all possible values for two cells.
  // Then we want the set the first cell's candidates to {1/2/7} and the
  // second's to {2/7/8}
  bool printed = false;
  for (unsigned i = 0; i < cage.cells.size(); ++i) {
    Cell *cell = cage.cells[i];
    Mask possibles_mask = 0u;
    for (auto const &cage_combo : *cage.cage_combos)
      for (auto const &perm : cage_combo.permutations)
        possibles_mask.set(perm[i] - 1);

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
  for (auto *cell : house.cells)
    for (auto *pcage : cell->all_cages())
      if (visited.insert(pcage).second)
        cage_list.push_back(pcage);

  for (auto *cage : cage_list) {
    std::unordered_set<const Cage *> other_visited;
    std::unordered_map<Mask, std::pair<Mask, CellCageUnit>> invalid_subsets;

    if (!cage->cage_combos)
      continue;

    auto &cage_combos = *cage->cage_combos;

    std::unordered_set<Mask> unique_combos =
        cage_combos.getUniqueCombinationsIn(house);

    for (auto *other_cell : house.cells) {
      if (cage->contains(other_cell))
        continue;

      // Check if this combination would invalidate all candidates for this
      // particular cell.
      if (std::all_of(
              std::begin(cage->cells), std::end(cage->cells),
              [other_cell](const Cell *c) { return c->canSee(other_cell); }))
        for (auto const &combo_mask : unique_combos)
          if ((combo_mask & other_cell->candidates) == other_cell->candidates)
            invalid_subsets[combo_mask] = {other_cell->candidates,
                                           CellCageUnit{other_cell}};

      // Else, try and determine a set of combinations that this other cell's
      // cage must have. For instance, we know that 14/2 must be {48|59} so
      // must include at least one of {48|49|58|59}. This would conflict with a
      // potential combination for {48} for cage 12/2, rendering it impossible.
      if (invalid_subsets.empty()) {
        for (auto *other_cage : other_cell->all_cages()) {
          if (!other_cage->cage_combos || other_cage->overlapsWith(cage))
            continue;

          // Construct the mask of cage cells which see cells in this cage.
          CellMask cage_cell_mask;
          if (other_cage->areAllCellsAlignedWith(house))
            cage_cell_mask.set();
          else
            for (unsigned i = 0, e = other_cage->size(); i != e; i++)
              cage_cell_mask[i] = house.contains((*other_cage)[i]);

          for (auto m : other_cage->cage_combos->computeKillerPairs(
                   static_cast<unsigned>(cage->size()), cage_cell_mask)) {
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
      size_t prev_size = cage_combos.combos.size();
      cage_combos.combos.erase(
          std::remove_if(std::begin(cage_combos), std::end(cage_combos),
                         [inv = invalid](CageCombo const &combo) {
                           return combo.duplicates.none() && combo.combo == inv;
                         }),
          std::end(cage_combos));
      if (prev_size != cage_combos.combos.size()) {
        if (debug) {
          const auto &[conflict_mask, conflict_unit] = conflict_data;
          dbgs() << "Conflicting Combos: cage " << *cage << " combination "
                 << printCandidateString(invalid) << " conflicts with "
                 << conflict_unit.getName() << " " << conflict_unit << " in "
                 << house << ", whose candidates must include at least one of "
                 << printCandidateString(conflict_mask) << "\n";
        }
        // Try and remove candidates from this cage's cells.
        if (runOnCage(*cage, debug, ""))
          return true;
      }
    }
  }

  return false;
}

// For the given combo_mask, check all of the cage's permutations containing
// the value overlap_candidate in the cage cell marked by overlapping_cell_idx,
// to see whether the combination specified by combo_mask is invalid.
static std::vector<IntList> getPermutationClashes(Mask combo_mask,
                                                  Cage const *cage,
                                                  CellMask cage_mask,
                                                  unsigned overlapping_cell_idx,
                                                  unsigned overlap_candidate) {
  std::vector<IntList> clashes;
  for (auto const &cage_combo : *cage->cage_combos) {
    for (auto const &other_perm : cage_combo.permutations) {
      // Only check permutations where the overlapping cell has the same value.
      if (other_perm[overlapping_cell_idx] != overlap_candidate)
        continue;
      Mask other_combo_mask =
          CageCombo::comboMaskFromPermuation(other_perm, cage_mask);
      // It's not a clash unless this permutation clashes with all
      // permutations containing these candidates.
      if (combo_mask != other_combo_mask)
        return {};
      clashes.push_back(other_perm);
    }
  }
  return clashes;
}

// | XX XY YY |
// | .. .. YY |
// This step checks combinations of cage X (containing two cells) against
// combinations of cage Y (containing three cells) where cell XY overlaps the
// two cages. If some combination of X would mean that it is impossible to fill
// in cage Y, we know that combination is invalid.
// Above, if cage X were 10/2 and cage Y were 17/3 then any combination where
// YY in row 2 is 7 is invalid, as there would be two 10/2 cages in the same
// row.
bool EliminateHardConflictingCombosStep::runOnHouse(House &house, bool debug) {
  std::unordered_set<const Cage *> visited;

  std::vector<Cage *> cage_list;
  for (auto *cell : house.cells)
    for (auto *pcage : cell->all_cages())
      if (pcage->cage_combos && visited.insert(pcage).second)
        cage_list.push_back(pcage);

  for (auto *cage : cage_list) {
    bool modified = false;
    std::vector<std::size_t> invalid_permutation_indices;
    std::unordered_set<const Cage *> other_visited;

    for (auto *cell : *cage) {
      for (auto *other_cage : cell->all_cages()) {
        if (cage == other_cage || !other_cage->cage_combos ||
            !other_visited.insert(other_cage).second)
          continue;

        CellMask cage_cell_mask = 0, other_cage_cell_mask = 0;
        std::vector<Cell *> overlaps;

        static std::array<char, 8> symbols = {
            '*', '+', '!', '\'', '"', '^', '&', '-',
        };
        std::unordered_map<unsigned, char> symbol_map, other_symbol_map;
        auto cage_it = std::begin(symbols);
        auto other_cage_it = std::begin(symbols);

        for (unsigned i = 0, e = cage->size(); i != e; i++) {
          Cell *c = (*cage)[i];
          if (!house.contains(c))
            continue;
          cage_cell_mask.set(i);
          if (cage_it == std::end(symbols))
            throw invalid_grid_exception{"Not enough symbols"};
          symbol_map[i] = *cage_it++;
          for (unsigned j = 0, je = other_cage->size(); j != je; j++)
            if ((*other_cage)[j] == c)
              overlaps.push_back(c);
        }

        for (unsigned i = 0, e = other_cage->size(); i != e; i++) {
          Cell *oc = (*other_cage)[i];
          if (!house.contains(oc))
            continue;
          other_cage_cell_mask.set(i);
          if (other_cage_it == std::end(symbols))
            throw invalid_grid_exception{"Not enough symbols"};
          other_symbol_map[i] = *other_cage_it++;
        }

        // TODO: Permit more overlaps?
        if (overlaps.size() != 1)
          continue;

        if (cage_cell_mask.count() < 2 || other_cage_cell_mask.count() < 2)
          continue;

        bool printed_banner = false;
        std::stringstream banner_ss;

        if (debug) {
          banner_ss << *cage << " overlaps " << *other_cage << " on cell(s) ("
                    << overlaps[0]->coord << ") in " << house << "\n";

          banner_ss << "Cells ";
          cage->printAnnotatedMaskedCellList(banner_ss, cage_cell_mask,
                                             symbol_map);
          banner_ss << " & ";
          other_cage->printAnnotatedMaskedCellList(
              banner_ss, other_cage_cell_mask, other_symbol_map);
          banner_ss << "\n";
        }

        unsigned overlap_cell_idx = *cage->indexOf(overlaps[0]);
        unsigned overlap_other_cell_idx = *other_cage->indexOf(overlaps[0]);

        // Check each cage combination
        for (auto &cage_combo : *cage->cage_combos) {
          std::stringstream ss;
          auto &permutations = cage_combo.permutations;
          // And manually check each permutation for ones which clash/overlap
          // with the other cage's permutations for the same values.
          for (unsigned p = 0, pe = permutations.size(); p != pe; p++) {
            auto const &permutation = permutations[p];
            Mask combo_mask =
                CageCombo::comboMaskFromPermuation(permutation, cage_cell_mask);
            auto clashes = getPermutationClashes(
                combo_mask, other_cage, other_cage_cell_mask,
                overlap_other_cell_idx, permutation[overlap_cell_idx]);
            if (clashes.empty())
              continue;
            if (debug) {
              ss << "\tConflicting cage combination:  ";
              printAnnotatedIntList(ss, permutation, symbol_map);
              ss << "\n\tClashing with:  ";
              printAnnotatedIntList(ss, clashes[0], other_symbol_map);
              if (clashes.size() > 1) {
                ss << " & ";
                if (clashes.size() > 2)
                  ss << "... & ";
                printAnnotatedIntList(ss, clashes[clashes.size() - 1],
                                      other_symbol_map);
              } else {
                bool sep = false;
                for (auto const &c : clashes) {
                  ss << (sep ? "," : "");
                  printAnnotatedIntList(ss, c, other_symbol_map);
                  sep = true;
                }
              }
              ss << "\n";
            }
            invalid_permutation_indices.push_back(p);
          }

          if (invalid_permutation_indices.empty())
            continue;

          if (debug) {
            if (!printed_banner)
              dbgs() << banner_ss.str();
            dbgs() << ss.str();
            printed_banner = true;
          }
          // First whittle down the permutations, then see if we can elimiate
          // cell candidates accordingly.
          remove_by_indices(permutations, invalid_permutation_indices);
          // Now see if this eliminates cell candidates.
          modified |=
              runOnCage(*cage, debug, "After removing this combination:\n");
          // Clear this vector for the next combo
          invalid_permutation_indices.clear();
        }
      }
    }

    // Only do one cage at a time...
    if (modified)
      return true;
  }

  return false;
}
