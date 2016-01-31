#include "defs.h"
#include "all_steps.h"

#include <memory>
#include <iostream>

bool DEBUG = false;
static bool USE_COLOUR = true;
static bool PRINT_AFTER_STEPS = false;

int main() {
  auto grid = std::make_unique<Grid>();

  if (grid->initialize()) {
    std::cout << "Invalid grid...\n";
    return 1;
  }

  std::cout << "Starting Out...\n";
  printGrid(grid.get(), USE_COLOUR);

  std::vector<std::unique_ptr<ColumboStep>> steps;
  steps.push_back(std::make_unique<EliminateImpossibleCombosStep>());
  steps.push_back(std::make_unique<EliminateNakedPairsStep>());
  steps.push_back(std::make_unique<EliminateNakedTriplesStep>());
  steps.push_back(std::make_unique<EliminateHiddenSinglesStep>());
  steps.push_back(std::make_unique<EliminateHiddenPairsStep>());
  steps.push_back(std::make_unique<EliminateHiddenTriplesStep>());
  steps.push_back(std::make_unique<EliminateCageUnitOverlapStep>());
  steps.push_back(std::make_unique<EliminatePointingPairsOrTriplesStep>());
  steps.push_back(std::make_unique<EliminateOneCellInniesAndOutiesStep>());
  steps.push_back(std::make_unique<PropagateFixedCells>());

  bool is_complete = false;
  bool done_something = true;

  int step_no = 0;
  for (; step_no < 15 && !is_complete && done_something; ++step_no) {
    done_something = false;
    for (auto &step : steps) {
      const bool modified = step->runOnGrid(grid.get());
      if (modified) {
        if (PRINT_AFTER_STEPS) {
          printGrid(grid.get(), USE_COLOUR, step->getName());
        }
      } else if (DEBUG) {
        std::cout << step->getName() << " did nothing...\n";
      }
      done_something |= modified;
    }

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
    std::cout << "Complete in " << step_no << " steps!\n";
  } else {
    std::cout << "Stuck after " << step_no << " steps!\n";
    return 1;
  }

  return 0;
}
