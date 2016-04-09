#include "cage_unit_overlap.h"

// Find cases where a candidate is defined in a cage and defined nowhere else
// in a row/column/box. All possible cage combinations without that number can
// be removed.
StepCode EliminateCageUnitOverlapStep::runOnHouse(House &house) {
  bool modified = false;

  CellCountMaskArray cell_masks;
  collectCellCountMaskInfo(house, cell_masks);

  for (unsigned i = 0, e = cell_masks.size(); i < e; ++i) {
    const Mask cell_mask = cell_masks[i];

    if (bitCount(cell_mask) == 1) {
      continue;
    }

    bool invalid = false;
    Cage *last_cage = nullptr;
    for (unsigned x = 0; x < 9 && !invalid; ++x) {
      const bool is_on = (cell_mask >> x) & 0x1;
      if (!is_on) {
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
      return {true, modified};
    }

    // Find the subset sums from the cage given each cell's current candidates
    std::vector<IntList> possibles;
    possibles.resize(last_cage->cells.size());

    unsigned idx = 0;
    for (auto &cell : last_cage->cells) {
      for (unsigned x = 0; x < 9; ++x) {
        if (cell->candidates[x]) {
          possibles[idx].push_back(x + 1);
        }
      }
      ++idx;
    }

    std::vector<IntList> subsets;
    generateSubsetSums(last_cage->sum, possibles, subsets);

    // Collect updated candidate masks. Start 'em all out at 0
    std::vector<Mask> new_masks(last_cage->cells.size(), 0);

    for (auto subset : subsets) {
      // This subset hasn't got our specific number in it. Filter it out.
      if (std::find(subset.begin(), subset.end(), i + 1) == subset.end()) {
        continue;
      }

      for (unsigned c = 0; c < last_cage->size(); ++c) {
        new_masks[c] |= (1 << (subset[c] - 1));
      }
    }

    for (unsigned c = 0; c < last_cage->size(); ++c) {
      const Mask mask = new_masks[c];
      Cell *cell = last_cage->cells[c];

      const Mask intersection = cell->candidates.to_ulong() & ~mask;

      if (intersection != 0) {
        if (DEBUG) {
          dbgs() << "Cage/Unit Overlap: " << i + 1 << " of "
                 << house.getPrintKind() << " " << getHousePrintNum(house)
                 << " overlaps w/ cage starting " << last_cage->cells[0]->coord
                 << "; removing " << printCandidateString(intersection)
                 << " from " << cell->coord << "\n";
        }
        modified = true;
        cell->candidates = CandidateSet(cell->candidates.to_ulong() & mask);
      }
    }
  }

  return {false, modified};
}
