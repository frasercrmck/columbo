#include "defs.h"

#include "init.h"
#include "singles.h"
#include "naked_pairs.h"
#include "killer_combos.h"

#include "utils.h"

#include <memory>
#include <iostream>

static bool performStep(Grid *const grid, bool progress, const char *message) {
  if (progress) {
    printGrid(grid, message);
    return true;
  }

  std::cout << message << " did nothing...\n";
  return false;
}

int main() {
  auto grid = std::make_unique<Grid>();

  HouseArray rows;
  HouseArray cols;
  HouseArray boxes;

  for (unsigned row = 0; row < 9; ++row) {
    for (unsigned col = 0; col < 9; ++col) {
      (*grid)[row][col].coord = {row, col};
      rows[row][col] = &(*grid)[row][col];
      cols[row][col] = &(*grid)[col][row];
    }
  }

  for (unsigned y = 0; y < 3; ++y) {
    for (unsigned x = 0; x < 3; ++x) {
      for (unsigned z = 0; z < 3; ++z) {
        for (unsigned w = 0; w < 3; ++w) {
          unsigned a = y * 3 + x;
          unsigned b = z * 3 + w;
          unsigned c = y * 3 + z;
          unsigned d = x * 3 + w;
          boxes[a][b] = &(*grid)[c][d];
        }
      }
    }
  }

  auto cages = CageList();

  init(grid.get(), cages);

  if (verify(grid.get(), cages)) {
    std::cout << "Grid failed to verify...\n";
    return 1;
  }

  std::cout << "Starting Out...\n";
  printGrid(grid.get());

  bool is_complete = false;
  for (int i = 0; i < 10 && !is_complete; ++i) {
    bool done_something = false;
    // Impossible Killer Combos
    done_something |= performStep(grid.get(), eliminateImpossibleCombos(cages),
                                  "Removing Impossible Combos");
    // Naked Pairs
    done_something |=
        performStep(grid.get(), eliminateNakedPairs(rows, cols, boxes, cages),
                    "Naked Pairs");

    // Cleaning up after previous steps
    done_something |= performStep(
        grid.get(), eliminateSingles(rows, cols, boxes), "Cleaning Up");

    is_complete = true;
    for (unsigned row = 0; row < 9 && is_complete; row++) {
      for (auto &cell : rows[row]) {
        if (!cell->isFixed()) {
          is_complete = false;
          break;
        }
      }
    }

    if (is_complete) {
      std::cout << "Complete!\n";
    } else if (!done_something) {
      std::cout << "Stuck!\n";
      return 1;
    }
  }

  return 0;
}
