#include "cage_unit_overlap.h"
#include <algorithm>
#include <numeric>
#include <unordered_set>

// Find cases where a candidate is defined in a cage and defined nowhere else
// in a row/column/box. All possible cage combinations without that number can
// be removed.
bool EliminateCageUnitOverlapStep::runOnHouse(House &house, bool debug) {
  bool modified = false;

  std::vector<Cage *> cage_list;
  std::unordered_set<Cage const *> visited;

  for (auto *cell : house.cells)
    for (auto *pcage : cell->all_cages())
      if (visited.insert(pcage).second && pcage->cage_combos)
        cage_list.push_back(pcage);

  std::vector<std::pair<Mask, Cage const *>> overlaps;
  for (auto const *cage : cage_list) {
    std::unordered_set<Mask> unique_combos =
        cage->cage_combos->getUniqueCombinationsIn(house);

    Mask combined;
    combined.set();

    for (auto const &subs : unique_combos)
      combined &= subs;

    if (combined.any())
      overlaps.push_back(std::make_pair(combined, cage));
  }

  for (auto &[mask, cage] : overlaps) {
    bool printed = false;
    for (auto *cell : house) {
      if (!cage->contains(cell)) {
        if (auto intersection = updateCell(cell, ~mask)) {
          modified |= true;
          if (debug) {
            if (!printed) {
              printed = true;
              dbgs() << "Cage/Unit Overlap: all combinations of cage " << *cage
                     << " aligned with " << house << " contain candidates "
                     << printCandidateString(mask) << ":\n";
            }
            dbgs() << "\tRemoving " << printCandidateString(*intersection)
                   << " from " << cell->coord << "\n";
          }
        }
      }
    }
  }

  if (modified)
    return modified;

  CellCountMaskArray cell_masks = collectCellCountMaskInfo(house);

  for (unsigned i = 0, e = cell_masks.size(); i < e; ++i) {
    const Mask cell_mask = cell_masks[i];

    if (cell_mask.count() == 1) {
      continue;
    }

    bool invalid = false;
    Cage *last_cage = nullptr;
    for (unsigned x = 0; x < 9 && !invalid; ++x) {
      if (!cell_mask[x]) {
        continue;
      }

      Cell *cell = house[x];

      invalid = last_cage && cell->cage != last_cage;
      last_cage = cell->cage;
    }

    if (invalid) {
      // This number is defined over more than one cage. Not interesting...
      continue;
    }

    if (!last_cage)
      // This number isn't a possibility at all in this cage. That's an error.
      throw invalid_grid_exception{};

    if (!last_cage->cage_combos)
      throw invalid_grid_exception{"Cages must have combo information"};

    auto &cage_combos = *last_cage->cage_combos;

    // Collect updated candidate masks. Start 'em all out at 0
    std::vector<Mask> new_masks(last_cage->cells.size(), 0);

    for (auto &cage_combo : cage_combos) {
      // This cage combo hasn't got our specific number in it. Filter it out.
      if (cage_combo.combo[i])
        for (auto const &perm : cage_combo.permutations)
          for (std::size_t c = 0, ce = last_cage->size(); c != ce; c++)
            new_masks[c] |= (1 << (perm[c] - 1));
    }

    for (unsigned c = 0; c < last_cage->size(); ++c) {
      const Mask mask = new_masks[c];
      Cell *cell = (*last_cage)[c];

      if (auto intersection = updateCell(cell, mask)) {
        modified = true;
        if (debug) {
          dbgs() << "Cage/Unit Overlap: " << i + 1 << " of " << house
                 << " overlaps w/ cage " << *last_cage << "; removing "
                 << printCandidateString(*intersection) << " from "
                 << cell->coord << "\n";
        }
      }
    }
  }

  return modified;
}
