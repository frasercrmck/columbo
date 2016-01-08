#ifndef SOLVER_HIDDENS_H
#define SOLVER_HIDDENS_H

#include "defs.h"
#include "utils.h"
#include "debug.h"

using CellCountMaskArray = std::array<unsigned long, 9>;

static void collectCellCountMaskInfo(House &house,
                                     CellCountMaskArray &cell_masks) {
  for (std::size_t i = 0; i < 9; ++i) {
    cell_masks[i] = 0;
  }
  for (auto &cell : house) {
    auto *candidates = &cell->candidates;
    for (std::size_t i = 0; i < 9; ++i) {
      if (candidates->test(i)) {
        cell_masks[i] |= (1 << house.getLinearID(cell));
      }
    }
  }
}

// Search a given house for a 'single': a cell that is the only that is the
// only in the house to potentially contain a value
bool eliminateHiddenSingles(House &house) {
  bool modified = false;

  CellCountMaskArray cell_masks;
  collectCellCountMaskInfo(house, cell_masks);

  for (unsigned i = 0, e = cell_masks.size(); i < e; ++i) {
    const unsigned long cell_mask = cell_masks[i];
    if (bitCount(cell_mask) != 1) {
      continue;
    }

    Cell *cell = nullptr;
    for (unsigned x = 0; x < 9; ++x) {
      const bool is_on = (cell_mask >> x) & 0x1;
      if (is_on) {
        cell = house[x];
        break;
      }
    }

    if (cell->isFixed()) {
      continue;
    }

    modified = true;
    const unsigned long mask = 1 << i;

    if (DEBUG) {
      dbgs() << "Hidden Singles: " << cell->coord << " set to " << (i + 1)
             << "; unique in " << house.getPrintKind() << "\n";
    }

    cell->candidates = mask;
  }

  return modified;
}

static bool eliminateHiddenSingles(HouseArray &rows, HouseArray &cols,
                                   HouseArray &boxes) {
  bool modified = false;
  for (auto &row : rows) {
    modified |= eliminateHiddenSingles(*row);
  }
  for (auto &col : cols) {
    modified |= eliminateHiddenSingles(*col);
  }
  for (auto &box : boxes) {
    modified |= eliminateHiddenSingles(*box);
  }
  return modified;
}

bool exposeHiddenCagePairs(House &house) {
  bool modified = false;

  CellCountMaskArray cell_masks;
  collectCellCountMaskInfo(house, cell_masks);

  for (unsigned i = 0, e = cell_masks.size(); i < e; ++i) {
    const unsigned long cell_mask = cell_masks[i];
    if (bitCount(cell_mask) != 2) {
      continue;
    }

    // We've found a hidden triple!
    std::array<Cell *, 2> pair_cells;

    unsigned idx = 0;
    for (unsigned x = 0; x < 9; ++x) {
      const bool is_on = (cell_mask >> x) & 0x1;
      if (!is_on) {
        continue;
      }

      pair_cells[idx++] = house[x];
    }

    bool is_interesting = true;
    Cell *const cell_0 = pair_cells[0];
    Cell *const cell_1 = pair_cells[1];

    // Don't care about fixed cells
    is_interesting &= (!cell_0->isFixed() && !cell_1->isFixed());
    // Don't care about cells in disjoint cells
    is_interesting &= cell_0->cage == cell_1->cage;
    // Don't care about cages larger than 2
    is_interesting &= cell_0->cage->size() == 2;
    // Don't care about naked pairs
    is_interesting &=
        (cell_0->candidates.count() != 2 && cell_1->candidates.count() != 2);

    if (!is_interesting) {
      continue;
    }

    if (DEBUG) {
      dbgs() << "Two-cell cage " << cell_0->coord << "/" << cell_1->coord
             << " must contain " << (i + 1) << ". ";
    }

    const int other_cell = cell_0->cage->sum - static_cast<int>(i + 1);

    const unsigned long mask = (1 << (other_cell - 1)) | (1 << i);

    modified = true;

    if (DEBUG) {
      dbgs() << "Setting " << cell_0->coord << " and " << cell_1->coord
             << " to " << printCandidateString(mask) << "\n";
    }

    cell_0->candidates = CandidateSet(mask);
    cell_1->candidates = CandidateSet(mask);
  }

  return modified;
}

static bool exposeHiddenCagePairs(HouseArray &rows, HouseArray &cols,
                                  HouseArray &boxes) {
  bool modified = false;
  for (auto &row : rows) {
    modified |= exposeHiddenCagePairs(*row);
  }
  for (auto &col : cols) {
    modified |= exposeHiddenCagePairs(*col);
  }
  for (auto &box : boxes) {
    modified |= exposeHiddenCagePairs(*box);
  }
  return modified;
}

struct TripleInfo {
  unsigned long cell_mask = 0;
  unsigned num_1 = 0;
  unsigned num_2 = 0;
  unsigned num_3 = 0;

  bool invalid = false;

  void addDef(unsigned i) {
    if (!num_1) {
      num_1 = i;
    } else if (!num_2) {
      num_2 = i;
    } else if (!num_3) {
      num_3 = i;
    } else {
      invalid = true;
    }
  }

  bool operator==(const TripleInfo &other) {
    return cell_mask == other.cell_mask && num_1 == other.num_1 &&
           num_2 == other.num_2 && num_3 == other.num_3;
  }

  bool operator!=(const TripleInfo &other) { return !operator==(other); }
};

bool eliminateHiddenTriples(House &house) {
  bool modified = false;

  CellCountMaskArray cell_masks;
  collectCellCountMaskInfo(house, cell_masks);

  std::vector<unsigned> interesting_numbers;
  for (unsigned i = 0, e = cell_masks.size(); i < e; ++i) {
    const int bit_count = bitCount(cell_masks[i]);
    if (bit_count == 1 || bit_count > 3) {
      continue;
    }
    interesting_numbers.push_back(i);
  }

  // If we haven't found enough interesting numbers - we need 3 for a triple -
  // then bail.
  if (interesting_numbers.size() < 3) {
    return modified;
  }

  std::vector<TripleInfo> masks;
  for (auto i : interesting_numbers) {
    const unsigned long cell_mask = cell_masks[i];

    // clang-format off
    auto found = std::find_if(masks.begin(), masks.end(),
                              [&cell_mask](TripleInfo info) {
      return info.cell_mask == cell_mask;
    });
    // clang-format on

    bool not_found = found == masks.end();

    for (unsigned f = 0, e = static_cast<unsigned>(masks.size()); f < e; ++f) {
      TripleInfo &found_mask = masks[f];
      // Try and create a composite cell array from these masks
      const unsigned long combined_cell_mask = cell_mask | found_mask.cell_mask;
      // We can immediately discard this if it creates something larger than a
      // triple
      if (bitCount(combined_cell_mask) > 3) {
        continue;
      }

      // clang-format off
      auto combined_found = std::find_if(masks.begin(), masks.end(),
                                        [&combined_cell_mask](TripleInfo info) {
        return info.cell_mask == combined_cell_mask;
      });
      // clang-format on

      // If we've found a new mask, record it and try the next
      if (combined_found == masks.end()) {
        TripleInfo new_info = found_mask;
        new_info.cell_mask = combined_cell_mask;
        new_info.addDef(i + 1);

        // Doesn't change e - won't search it again
        masks.push_back(new_info);
        continue;
      }

      // If this is creating an new mask (say (101 | 110) = 111) then don't add
      // info to the two-cell mask.
      if (*combined_found != found_mask) {
        continue;
      }

      // Else, add a new candidate number to this 2-or-3-sized group
      found_mask.addDef(i + 1);
    }

    if (not_found) {
      TripleInfo new_info;
      new_info.cell_mask = cell_mask;
      new_info.addDef(i + 1);
      masks.push_back(new_info);
    }
  }

  for (auto &mask : masks) {
    if (mask.invalid) {
      continue;
    }

    if (mask.num_3 == 0) {
      continue;
    }

    // We've found a hidden triple!
    std::array<Cell *, 3> triple_cells;

    unsigned idx = 0;
    for (unsigned x = 0; x < 9; ++x) {
      const bool is_on = (mask.cell_mask >> x) & 0x1;
      if (!is_on) {
        continue;
      }

      triple_cells[idx++] = house[x];
    }

    const unsigned long triple_candidate_mask = (1 << (mask.num_1 - 1)) |
                                                (1 << (mask.num_2 - 1)) |
                                                (1 << (mask.num_3 - 1));

    for (Cell *cell : triple_cells) {
      auto *candidates = &cell->candidates;

      const unsigned long intersection_mask =
          candidates->to_ulong() & ~triple_candidate_mask;
      if (intersection_mask != 0) {
        modified = true;
        if (DEBUG) {
          dbgs() << "Hidden Triple "
                 << printCandidateString(triple_candidate_mask) << " in cells "
                 << triple_cells[0]->coord << "/" << triple_cells[1]->coord
                 << "/" << triple_cells[2]->coord << " removes "
                 << printCandidateString(intersection_mask) << " from "
                 << cell->coord << "\n";
        }
        *candidates =
            CandidateSet(candidates->to_ulong() & triple_candidate_mask);
      }
    }
  }

  return modified;
}

static bool eliminateHiddenTriples(HouseArray &rows, HouseArray &cols,
                                   HouseArray &boxes) {
  bool modified = false;
  for (auto &row : rows) {
    modified |= eliminateHiddenTriples(*row);
  }
  for (auto &col : cols) {
    modified |= eliminateHiddenTriples(*col);
  }
  for (auto &box : boxes) {
    modified |= eliminateHiddenTriples(*box);
  }
  return modified;
}

#endif // SOLVER_HIDDENS_H
