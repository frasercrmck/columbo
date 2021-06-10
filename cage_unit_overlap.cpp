#include "cage_unit_overlap.h"
#include <algorithm>
#include <numeric>
#include <unordered_set>

// Find cases where a candidate is defined in a cage and defined nowhere else
// in a row/column/box. All possible cage combinations without that number can
// be removed.
bool EliminateCageUnitOverlapStep::runOnHouse(House &house) {
  bool modified = false;

  std::unordered_set<Cage const *> visited;
  std::vector<std::pair<Mask, Cage const *>> overlaps;
  for (auto const *cell : house) {
    if (!visited.insert(cell->cage).second)
      continue;
    if (!cell->cage->areAllCellsAlignedWith(house))
      continue;

    Mask combined;
    combined.set();

    for (auto const &subs : *cell->cage->cage_combos)
      combined &= subs.combo;

    if (combined.any())
      overlaps.push_back(std::make_pair(combined, cell->cage));
  }

  for (auto &[mask, cage] : overlaps) {
    bool printed = false;
    for (auto *cell : house) {
      if (cell->cage != cage) {
        if (auto intersection = updateCell(cell, ~mask)) {
          modified |= true;
          if (DEBUG) {
            if (!printed) {
              printed = true;
              dbgs() << "Cage/Unit Overlap: all combinations of cage " << *cage
                     << " aligned with " << getHousePrintNum(house)
                     << " contain candidates " << printCandidateString(mask)
                     << ":\n";
            }
            dbgs() << "Removing " << printCandidateString(*intersection)
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

    std::vector<CageCombo> &subsets = *last_cage->cage_combos;

    // Collect updated candidate masks. Start 'em all out at 0
    std::vector<Mask> new_masks(last_cage->cells.size(), 0);

    for (auto &subset : subsets) {
      // This subset hasn't got our specific number in it. Filter it out.
      if (subset.combo[i])
        for (auto const &perm : subset.permutations)
          for (unsigned c = 0, ce = last_cage->size(); c != ce; c++)
            new_masks[c] |= (1 << (perm[c] - 1));
    }

    for (unsigned c = 0; c < last_cage->size(); ++c) {
      const Mask mask = new_masks[c];
      Cell *cell = last_cage->cells[c];

      if (auto intersection = updateCell(cell, mask)) {
        modified = true;
        if (DEBUG) {
          dbgs() << "Cage/Unit Overlap: " << i + 1 << " of "
                 << house.getPrintKind() << " " << getHousePrintNum(house)
                 << " overlaps w/ cage " << *last_cage << "; removing "
                 << printCandidateString(*intersection) << " from "
                 << cell->coord << "\n";
        }
      }
    }
  }

  return modified;
}
