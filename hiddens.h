#ifndef SOLVER_HIDDENS_H
#define SOLVER_HIDDENS_H

#include "defs.h"
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
      dbgs() << cell->coord << " set to " << (i + 1) << "; unique in house\n";
    }

    cell->candidates = mask;
  }

  return modified;
}

static bool eliminateHiddenSingles(HouseArray &rows, HouseArray &cols,
                                   HouseArray &boxes) {
  bool modified = false;
  for (auto &row : rows) {
    modified |= eliminateHiddenSingles(row);
  }
  for (auto &col : cols) {
    modified |= eliminateHiddenSingles(col);
  }
  for (auto &box : boxes) {
    modified |= eliminateHiddenSingles(box);
  }
  return modified;
}

#endif // SOLVER_HIDDENS_H
