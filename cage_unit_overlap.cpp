#include "cage_unit_overlap.h"
#include <algorithm>

// Find cases where a candidate is defined in a cage and defined nowhere else
// in a row/column/box. All possible cage combinations without that number can
// be removed.
bool EliminateCageUnitOverlapStep::runOnHouse(House &house) {
  bool modified = false;
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

    if (!last_cage) {
      // This number isn't a possibility at all in this cage. That's an error.
      throw invalid_grid_exception{};
    }

    std::vector<CageCombo> &subsets = (*cage_combo_map)[last_cage];

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
                 << " overlaps w/ cage starting " << last_cage->cells[0]->coord
                 << "; removing " << printCandidateString(*intersection)
                 << " from " << cell->coord << "\n";
        }
      }
    }
  }

  return modified;
}
