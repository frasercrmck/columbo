#ifndef SOLVER_HIDDENS_H
#define SOLVER_HIDDENS_H

#include "defs.h"
#include "utils.h"
#include "debug.h"

using CellCountArray = std::array<std::vector<Cell *>, 9>;

// An array of vectors of Cells that could possibly be each value 1 - 9.
static void collectCellCountInfo(House &house, CellCountArray &cell_counts) {
  for (auto &cell : house) {
    auto *candidates = &cell->candidates;
    for (std::size_t i = 0; i < 9; ++i) {
      if (candidates->test(i)) {
        cell_counts[i].push_back(cell);
      }
    }
  }
}

// Search a given house for a 'single': a cell that is the only that is the
// only in the house to potentially contain a value
bool eliminateHiddenSingles(House &house) {
  bool modified = false;

  CellCountArray cell_counts;
  collectCellCountInfo(house, cell_counts);

  for (unsigned i = 0, e = cell_counts.size(); i < e; ++i) {
    auto &count_vec = cell_counts[i];
    if (count_vec.size() != 1) {
      continue;
    }

    Cell *cell = count_vec[0];

    if (cell->isFixed()) {
      continue;
    }

    modified = true;
    const unsigned long mask = 1 << i;

    if (DEBUG) {
      dbgs() << cell->coord << " set to " << (i + 1) << "; unique in "
             << house.getPrintKind() << "\n";
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

  CellCountArray cell_counts;
  collectCellCountInfo(house, cell_counts);

  for (unsigned i = 0, e = cell_counts.size(); i < e; ++i) {
    auto &count_vec = cell_counts[i];
    if (count_vec.size() != 2) {
      continue;
    }

    bool is_interesting = true;
    Cell *const cell_0 = count_vec[0];
    Cell *const cell_1 = count_vec[1];

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

    dbgs() << "Two-cell cage " << cell_0->coord << "/" << cell_1->coord
           << " must contain " << (i + 1) << ". ";

    const int other_cell = cell_0->cage->sum - static_cast<int>(i + 1);

    const unsigned long mask = (1 << (other_cell - 1)) | (1 << i);

    modified = true;

    dbgs() << "Setting " << cell_0->coord << " and " << cell_1->coord << " to "
           << printCandidateString(mask) << "\n";

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

#endif // SOLVER_HIDDENS_H
