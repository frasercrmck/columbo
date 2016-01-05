#ifndef SOLVER_HIDDENS_H
#define SOLVER_HIDDENS_H

#include "defs.h"
#include "debug.h"

// Search a given house for a 'single': a cell that is the only that is the
// only in the house to potentially contain a value
bool eliminateHiddenSingles(House &house) {
  bool modified = false;

  // An array of last-defined Cell with the count, for each value 1 - 9.
  // If the count is 1 - a single - then the Cell will point to the only
  // definition.
  std::array<std::pair<Cell *, unsigned>, 9> cell_counts;
  for (auto &cell : house) {
    auto *candidates = &cell->candidates;
    for (std::size_t i = 0; i < 9; ++i) {
      if (candidates->test(i)) {
        cell_counts[i].first = cell;
        cell_counts[i].second++;
      }
    }
  }

  for (unsigned i = 0, e = cell_counts.size(); i < e; ++i) {
    auto &count_pair = cell_counts[i];
    if (count_pair.second != 1 || count_pair.first->isFixed()) {
      continue;
    }

    modified = true;
    Cell *cell = count_pair.first;
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
