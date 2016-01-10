#include "defs.h"

#include "hiddens.h"
#include "nakeds.h"
#include "intersections.h"
#include "killer_combos.h"
#include "innies_outies.h"
#include "cage_unit_overlap.h"
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

  if (grid->initialize()) {
    std::cout << "Invalid grid...\n";
    return 1;
  }

  if (verify(grid.get(), grid->cages)) {
    std::cout << "Grid failed to verify...\n";
    return 1;
  }

  std::cout << "Starting Out...\n";
  printGrid(grid.get(), USE_COLOUR);

  bool is_complete = false;
  bool done_something = true;
  int step = 0;
  for (; step < 15 && !is_complete && done_something; ++step) {
    done_something = false;
    // Impossible Killer Combos
    done_something |=
        performStep(grid.get(), eliminateImpossibleCombos(grid.get()),
                    "Removing Impossible Combos");
    // Naked Pairs
    done_something |=
        performStep(grid.get(), eliminateNakedPairs(grid.get()), "Naked Pairs");

    // Naked Triples
    done_something |= performStep(grid.get(), eliminateNakedTriples(grid.get()),
                                  "Naked Triples");

    // Hidden Singles
    done_something |= performStep(
        grid.get(), eliminateHiddenSingles(grid.get()), "Hidden Singles");
    // Hidden Pairs
    done_something |= performStep(grid.get(), eliminateHiddenPairs(grid.get()),
                                  "Hidden Pairs");
    // Hidden Triples
    done_something |= performStep(
        grid.get(), eliminateHiddenTriples(grid.get()), "Hidden Triples");
    // Cage/Unit Overlap
    done_something |= performStep(
        grid.get(), eliminateCageUnitOverlap(grid.get()), "Hidden Cage Pairs");

    // Pointing Pairs/Triples
    done_something |=
        performStep(grid.get(), eliminatePointingPairsOrTriples(grid.get()),
                    "Pointing Pairs/Triples");

    // Innies & Outies
    done_something |= performStep(
        grid.get(), eliminateOneCellInniesAndOuties(grid.get()),
        "Innies & Outies (One Cell)");

    // Cleaning up after previous steps
    done_something |=
        performStep(grid.get(), propagateFixedCells(grid.get()), "Cleaning Up");

    is_complete = true;
    for (unsigned row = 0; row < 9 && is_complete; row++) {
      for (auto &cell : *(grid->rows[row])) {
        if (!cell->isFixed()) {
          is_complete = false;
          break;
        }
      }
    }
  }

  printGrid(grid.get(), USE_COLOUR);

  if (is_complete) {
    std::cout << "Complete in " << step << " steps!\n";
  } else {
    std::cout << "Stuck after " << step << " steps!\n";
    return 1;
  }

  return 0;
}
