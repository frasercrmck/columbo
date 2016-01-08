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

  initCages(grid.get());

  if (verify(grid.get(), grid->cages)) {
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
    // Hidden Cage Pairs
    done_something |= performStep(grid.get(), exposeHiddenCagePairs(grid.get()),
                                  "Hidden Cage Pairs");

    // Pointing Pairs/Triples
    done_something |=
        performStep(grid.get(), eliminatePointingPairsOrTriples(grid.get()),
                    "Pointing Pairs/Triples");

    // Innies & Outies
    done_something |= performStep(
        grid.get(), eliminateOneCellInniesAndOuties(innies_and_outies),
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
    std::cout << "Complete!\n";
  } else {
    std::cout << "Stuck!\n";
    return 1;
  }

  return 0;
}
