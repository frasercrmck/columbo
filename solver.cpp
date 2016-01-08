#include "defs.h"

#include "init.h"
#include "hiddens.h"
#include "nakeds.h"
#include "intersections.h"
#include "killer_combos.h"
#include "innies_outies.h"
#include "fixed_cell_cleanup.h"

#include "utils.h"

#include <memory>
#include <iostream>

static bool USE_COLOUR = true;
static bool PRINT_AFTER_STEPS = false;

static bool performStep(Grid *const grid, bool progress, const char *message) {
  if (progress) {
    if (PRINT_AFTER_STEPS) {
      printGrid(grid, USE_COLOUR, message);
    }
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

  for (unsigned i = 0; i < 9; ++i) {
    rows[i] = std::make_unique<Row>(i);
    cols[i] = std::make_unique<Col>(i);
    boxes[i] = std::make_unique<Box>(i);
  }

  for (unsigned row = 0; row < 9; ++row) {
    for (unsigned col = 0; col < 9; ++col) {
      (*grid)[row][col].coord = {row, col};
      (*rows[row])[col] = &(*grid)[row][col];
      (*cols[row])[col] = &(*grid)[col][row];
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
          (*boxes[a])[b] = &(*grid)[c][d];
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
  printGrid(grid.get(), USE_COLOUR);

  std::vector<std::unique_ptr<InnieOutieRegion>> innies_and_outies;

  initializeInnieAndOutieRegions(grid.get(), innies_and_outies);

  bool is_complete = false;
  bool done_something = true;
  for (int i = 0; i < 15 && !is_complete && done_something; ++i) {
    done_something = false;
    // Impossible Killer Combos
    done_something |= performStep(grid.get(), eliminateImpossibleCombos(cages),
                                  "Removing Impossible Combos");
    // Naked Pairs
    done_something |= performStep(
        grid.get(), eliminateNakedPairs(rows, cols, boxes), "Naked Pairs");

    // Naked Triples
    done_something |= performStep(
        grid.get(), eliminateNakedTriples(rows, cols, boxes), "Naked Triples");

    // Hidden Singles
    done_something |=
        performStep(grid.get(), eliminateHiddenSingles(rows, cols, boxes),
                    "Hidden Singles");
    // Hidden Pairs
    done_something |= performStep(
        grid.get(), eliminateHiddenPairs(rows, cols, boxes), "Hidden Pairs");
    // Hidden Triples
    done_something |=
        performStep(grid.get(), eliminateHiddenTriples(rows, cols, boxes),
                    "Hidden Triples");
    // Hidden Cage Pairs
    done_something |=
        performStep(grid.get(), exposeHiddenCagePairs(rows, cols, boxes),
                    "Hidden Cage Pairs");

    // Pointing Pairs/Triples
    done_something |= performStep(
        grid.get(), eliminatePointingPairsOrTriples(rows, cols, boxes),
        "Pointing Pairs/Triples");

    // Innies & Outies
    done_something |= performStep(
        grid.get(), eliminateOneCellInniesAndOuties(innies_and_outies),
        "Innies & Outies (One Cell)");

    // Cleaning up after previous steps
    done_something |= performStep(
        grid.get(), propagateFixedCells(rows, cols, boxes), "Cleaning Up");

    is_complete = true;
    for (unsigned row = 0; row < 9 && is_complete; row++) {
      for (auto &cell : *(rows[row])) {
        if (!cell->isFixed()) {
          is_complete = false;
          break;
        }
      }
    }
  }

  printGrid(grid.get(), USE_COLOUR);

  if (is_complete) {
    std::cout << "Complete!\n";
  } else {
    std::cout << "Stuck!\n";
    return 1;
  }

  return 0;
}
