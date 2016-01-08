#ifndef SOLVER_HIDDENS_H
#define SOLVER_HIDDENS_H

#include "defs.h"
#include "utils.h"
#include "debug.h"

// Search a given house for a 'single': a cell that is the only that is the
// only in the house to potentially contain a value
bool eliminateHiddenSingles(House &house) {
  bool modified = false;

  CellCountMaskArray cell_masks;
  collectCellCountMaskInfo(house, cell_masks);

  for (unsigned i = 0, e = cell_masks.size(); i < e; ++i) {
    const Mask cell_mask = cell_masks[i];
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
    const Mask mask = 1 << i;

    if (DEBUG) {
      dbgs() << "Hidden Singles: " << cell->coord << " set to " << (i + 1)
             << "; unique in " << house.getPrintKind() << "\n";
    }

    cell->candidates = mask;
  }

  return modified;
}

static bool eliminateHiddenSingles(Grid *const grid) {
  bool modified = false;
  for (auto &row : grid->rows) {
    modified |= eliminateHiddenSingles(*row);
  }
  for (auto &col : grid->cols) {
    modified |= eliminateHiddenSingles(*col);
  }
  for (auto &box : grid->boxes) {
    modified |= eliminateHiddenSingles(*box);
  }
  return modified;
}

bool exposeHiddenCagePairs(House &house) {
  bool modified = false;

  CellCountMaskArray cell_masks;
  collectCellCountMaskInfo(house, cell_masks);

  for (unsigned i = 0, e = cell_masks.size(); i < e; ++i) {
    const Mask cell_mask = cell_masks[i];
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

    const Mask mask = (1 << (other_cell - 1)) | (1 << i);

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

static bool exposeHiddenCagePairs(Grid *const grid) {
  bool modified = false;
  for (auto &row : grid->rows) {
    modified |= exposeHiddenCagePairs(*row);
  }
  for (auto &col : grid->cols) {
    modified |= exposeHiddenCagePairs(*col);
  }
  for (auto &box : grid->boxes) {
    modified |= exposeHiddenCagePairs(*box);
  }
  return modified;
}

struct PairInfo {
  Mask cell_mask = 0;
  unsigned num_1 = 0;
  unsigned num_2 = 0;

  bool invalid = false;

  void addDef(unsigned i) {
    if (!num_1) {
      num_1 = i;
    } else if (!num_2) {
      num_2 = i;
    } else {
      invalid = true;
    }
  }

  bool isFull() const { return num_2 != 0; }

  const char *getName() const { return "Pair"; }

  Mask generateNumMask() const {
    return (1 << (num_1 - 1)) | (1 << (num_2 - 1));
  }

  bool operator==(const PairInfo &other) {
    return cell_mask == other.cell_mask && num_1 == other.num_1 &&
           num_2 == other.num_2;
  }

  bool operator!=(const PairInfo &other) { return !operator==(other); }
};

struct TripleInfo {
  Mask cell_mask = 0;
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

  bool isFull() const { return num_3 != 0; }

  const char *getName() const { return "Triple"; }

  Mask generateNumMask() const {
    return (1 << (num_1 - 1)) | (1 << (num_2 - 1)) | (1 << (num_3 - 1));
  }

  bool operator==(const TripleInfo &other) {
    return cell_mask == other.cell_mask && num_1 == other.num_1 &&
           num_2 == other.num_2 && num_3 == other.num_3;
  }

  bool operator!=(const TripleInfo &other) { return !operator==(other); }
};

template <typename HiddenInfo, int Size> bool eliminateHiddens(House &house) {
  bool modified = false;

  CellCountMaskArray cell_masks;
  collectCellCountMaskInfo(house, cell_masks);

  std::vector<unsigned> interesting_numbers;
  for (unsigned i = 0, e = cell_masks.size(); i < e; ++i) {
    const int bit_count = bitCount(cell_masks[i]);
    if (bit_count == 1 || bit_count > Size) {
      continue;
    }
    interesting_numbers.push_back(i);
  }

  // If we haven't found enough interesting numbers then bail.
  if (interesting_numbers.size() < Size) {
    return modified;
  }

  std::vector<HiddenInfo> hidden_infos;
  for (auto i : interesting_numbers) {
    const Mask cell_mask = cell_masks[i];

    // clang-format off
    auto iter = std::find_if(hidden_infos.begin(), hidden_infos.end(),
                              [&cell_mask](HiddenInfo info) {
      return info.cell_mask == cell_mask;
    });
    // clang-format on

    bool not_found = iter == hidden_infos.end();

    const unsigned old_size = static_cast<unsigned>(hidden_infos.size());
    for (unsigned f = 0; f < old_size; ++f) {
      HiddenInfo &hidden_info = hidden_infos[f];
      // Try and create a composite cell array from these hidden_infos
      const Mask combined_cell_mask = cell_mask | hidden_info.cell_mask;
      // We can immediately discard this if it creates something larger than
      // the construction we're looking for
      if (bitCount(combined_cell_mask) > Size) {
        continue;
      }

      // clang-format off
      auto combined_found = std::find_if(hidden_infos.begin(), hidden_infos.end(),
                                        [&combined_cell_mask](HiddenInfo info) {
        return info.cell_mask == combined_cell_mask;
      });
      // clang-format on

      // If we've found a new mask, record it and try the next
      if (combined_found == hidden_infos.end()) {
        HiddenInfo new_info = hidden_info;
        new_info.cell_mask = combined_cell_mask;
        new_info.addDef(i + 1);

        // Doesn't change e - won't search it again
        hidden_infos.push_back(new_info);
        continue;
      }

      // If this is creating an new mask (say (101 | 110) = 111) then don't add
      // info to the old mask
      if (*combined_found != hidden_info) {
        continue;
      }

      // Else, add a new candidate number to this group
      hidden_info.addDef(i + 1);
    }

    if (not_found) {
      HiddenInfo new_info;
      new_info.cell_mask = cell_mask;
      new_info.addDef(i + 1);
      hidden_infos.push_back(new_info);
    }
  }

  for (auto &hidden : hidden_infos) {
    if (hidden.invalid || !hidden.isFull()) {
      continue;
    }

    const Mask hidden_candidate_mask = hidden.generateNumMask();

    // We've found a hidden pair/triple/quad!
    for (unsigned x = 0; x < 9; ++x) {
      const bool is_on = (hidden.cell_mask >> x) & 0x1;
      if (!is_on) {
        continue;
      }

      Cell *cell = house[x];
      auto *candidates = &cell->candidates;

      const Mask intersection_mask =
          candidates->to_ulong() & ~hidden_candidate_mask;
      if (intersection_mask != 0) {
        modified = true;
        if (DEBUG) {
          dbgs() << "Hidden " << hidden.getName() << " "
                 << printCandidateString(hidden_candidate_mask) << " in cells ";
          printCellMask(house, hidden.cell_mask);
          dbgs() << " removes " << printCandidateString(intersection_mask)
                 << " from " << cell->coord << "\n";
        }
        *candidates =
            CandidateSet(candidates->to_ulong() & hidden_candidate_mask);
      }
    }
  }

  return modified;
}

static bool eliminateHiddenPairs(Grid *const grid) {
  bool modified = false;
  for (auto &row : grid->rows) {
    modified |= eliminateHiddens<PairInfo, 2>(*row);
  }
  for (auto &col : grid->cols) {
    modified |= eliminateHiddens<PairInfo, 2>(*col);
  }
  for (auto &box : grid->boxes) {
    modified |= eliminateHiddens<PairInfo, 2>(*box);
  }
  return modified;
}

static bool eliminateHiddenTriples(Grid *const grid) {
  bool modified = false;
  for (auto &row : grid->rows) {
    modified |= eliminateHiddens<TripleInfo, 3>(*row);
  }
  for (auto &col : grid->cols) {
    modified |= eliminateHiddens<TripleInfo, 3>(*col);
  }
  for (auto &box : grid->boxes) {
    modified |= eliminateHiddens<TripleInfo, 3>(*box);
  }
  return modified;
}

#endif // SOLVER_HIDDENS_H
