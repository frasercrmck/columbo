#ifndef COLUMBO_HIDDENS_H
#define COLUMBO_HIDDENS_H

#include "debug.h"
#include "defs.h"
#include "step.h"
#include "utils.h"

#include <algorithm>

struct EliminateHiddenSinglesStep : ColumboStep {
  StepCode runOnGrid(Grid *const grid) override {
    changed.clear();
    StepCode ret = {false, false};
    for (auto &row : grid->rows) {
      ret |= runOnHouse(*row);
      if (ret) {
        return ret;
      }
    }
    for (auto &col : grid->cols) {
      ret |= runOnHouse(*col);
      if (ret) {
        return ret;
      }
    }
    for (auto &box : grid->boxes) {
      ret |= runOnHouse(*box);
      if (ret) {
        return ret;
      }
    }
    return ret;
  }

  virtual void anchor() override;

  const char *getID() const override { return "hidden-singles"; }
  const char *getName() const override { return "Hidden Singles"; }

private:
  StepCode runOnHouse(House &house);
};

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

template <typename HiddenInfo, int Size>
struct PairsOrTriplesStep : ColumboStep {
  StepCode runOnGrid(Grid *const grid) override {
    changed.clear();
    StepCode ret = {false, false};
    for (auto &row : grid->rows) {
      ret |= eliminateHiddens(*row);
      if (ret) {
        return ret;
      }
    }
    for (auto &col : grid->cols) {
      ret |= eliminateHiddens(*col);
      if (ret) {
        return ret;
      }
    }
    for (auto &box : grid->boxes) {
      ret |= eliminateHiddens(*box);
      if (ret) {
        return ret;
      }
    }
    return ret;
  }

protected:
  StepCode eliminateHiddens(House &house);
};

template <typename HiddenInfo, int Size>
StepCode PairsOrTriplesStep<HiddenInfo, Size>::eliminateHiddens(House &house) {
  bool modified = false;

  CellCountMaskArray cell_masks = collectCellCountMaskInfo(house);

  std::vector<unsigned> interesting_numbers;
  for (unsigned i = 0, e = cell_masks.size(); i < e; ++i) {
    const std::size_t bit_count = cell_masks[i].count();
    if (bit_count == 0) {
      return {true, modified};
    }
    if (bit_count == 1 || bit_count > Size) {
      continue;
    }
    interesting_numbers.push_back(i);
  }

  // If we haven't found enough interesting numbers then bail.
  if (interesting_numbers.size() < Size) {
    return {false, modified};
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
      if (combined_cell_mask.count() > Size) {
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
      if (!hidden.cell_mask[x]) {
        continue;
      }

      Cell *cell = house[x];
      const Mask intersection_mask = cell->candidates & ~hidden_candidate_mask;
      if (intersection_mask == 0) {
        continue;
      }

      if (DEBUG) {
        dbgs() << "Hidden " << hidden.getName() << " "
               << printCandidateString(hidden_candidate_mask) << " in cells "
               << printCellMask(house, hidden.cell_mask) << " removes "
               << printCandidateString(intersection_mask) << " from "
               << cell->coord << "\n";
      }

      modified = true;
      changed.insert(cell);
      cell->candidates &= hidden_candidate_mask;
    }
  }

  return {false, modified};
}

struct EliminateHiddenPairsStep : public PairsOrTriplesStep<PairInfo, 2> {
  virtual void anchor() override;

  const char *getID() const override { return "hidden-pairs"; }
  const char *getName() const override { return "Hidden Pairs"; }
};

struct EliminateHiddenTriplesStep : public PairsOrTriplesStep<TripleInfo, 3> {
  virtual void anchor() override;

  const char *getID() const override { return "hidden-triples"; }
  const char *getName() const override { return "Hidden Triples"; }
};

#endif // COLUMBO_HIDDENS_H
