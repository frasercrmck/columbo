#include "defs.h"

#include "init.h"
#include "singles.h"
#include "hiddens.h"
#include "nakeds.h"
#include "killer_combos.h"
#include "innies_outies.h"

#include "utils.h"

#include <memory>
#include <iostream>

static bool performStep(Grid *const grid, bool progress, bool use_colour,
                        const char *message) {
  if (progress) {
    printGrid(grid, use_colour, message);
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

  const bool use_colour = true;

  std::cout << "Starting Out...\n";
  printGrid(grid.get(), use_colour);

  std::vector<std::unique_ptr<InnieOutieRegion>> innies_and_outies;

  initializeInnieAndOutieRegions(grid.get(), innies_and_outies);

  bool is_complete = false;
  bool done_something = true;
  for (int i = 0; i < 10 && !is_complete && done_something; ++i) {
    done_something = false;
    // Impossible Killer Combos
    done_something |= performStep(grid.get(), eliminateImpossibleCombos(cages),
                                  use_colour, "Removing Impossible Combos");
    // Naked Pairs
    done_something |=
        performStep(grid.get(), eliminateNakedPairs(rows, cols, boxes),
                    use_colour, "Naked Pairs");

    // Naked Triples
    done_something |=
        performStep(grid.get(), eliminateNakedTriples(rows, cols, boxes),
                    use_colour, "Naked Triples");

    // Hidden Singles
    done_something |=
        performStep(grid.get(), eliminateHiddenSingles(rows, cols, boxes),
                    use_colour, "Hidden Singles");

    // Innies & Outies
    done_something |= performStep(
        grid.get(), eliminateOneCellInniesAndOuties(innies_and_outies),
        use_colour, "Innies & Outies (One Cell)");

    // Cleaning up after previous steps
    done_something |=
        performStep(grid.get(), eliminateSingles(rows, cols, boxes), use_colour,
                    "Cleaning Up");

    is_complete = true;
    for (unsigned row = 0; row < 9 && is_complete; row++) {
      for (auto &cell : rows[row]) {
        if (!cell->isFixed()) {
          is_complete = false;
          break;
        }
      }
    }
  }

  printGrid(grid.get(), use_colour);

  if (is_complete) {
    std::cout << "Complete!\n";
  } else {
    std::cout << "Stuck!\n";
    return 1;
  }

  return 0;
}
