#include "all_steps.h"
#include "defs.h"

#include <iostream>
#include <memory>

bool DEBUG = false;
static bool USE_COLOUR = true;
static bool PRINT_AFTER_STEPS = false;
static bool TIME = false;

static void print_help() {
  std::cout << R"(
    COLUMBO: A solver of killers. Of killer sudokus.

    Usage:
      columbo [option]

    Options:
      -h                        Print help and exit
      -p    --print-after-all   Print grid after every step
      -t    --time              Print detailed timing information
      -d    --debug             Print debug text for every step
            --no-colour         Don't print grids using colour

  )";
}
  // Helper to facilitate printing of the grid after each step
static void printGridIfNeeded(const Grid *grid, const ColumboStep *step,
                              const bool modified) {
  if (modified) {
    if (PRINT_AFTER_STEPS) {
      printGrid(grid, USE_COLOUR, step->getName());
    }
  } else if (DEBUG) {
    std::cout << step->getName() << " did nothing...\n";
  }
}

// Note: 'progress' is updated, not overwritten
StepCode runStep(Grid *grid, ColumboStep *step) {
  auto start = std::chrono::steady_clock::now();
  StepCode ret = step->runOnGrid(grid);

  if (ret) {
    return ret;
  }

  if (TIME) {
    auto end = std::chrono::steady_clock::now();
    auto diff_ms =
        std::chrono::duration<double, std::milli>(end - start).count();
    std::cout << step->getName() << " took " << diff_ms << "ms...\n";
  }

  printGridIfNeeded(grid, step, ret.modified);
  return ret;
}

bool solveGrid(Grid *const grid, bool &is_complete, unsigned &step_no) {
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

  auto cleanup_step = std::make_unique<PropagateFixedCells>();

  bool keep_going = true;
  while (keep_going) {
    ++step_no;
    StepCode ret = {false, false};
    for (auto &step : steps) {
      ret |= runStep(grid, step.get());
      if (ret) {
        return true;
      }

      auto changed = step->getChanged();

      if (changed.empty()) {
        continue;
      }

      // Do some fixed-cell cleanup
      cleanup_step->setWorkList(changed);
      ret |= runStep(grid, cleanup_step.get());

      if (ret) {
        return true;
      }
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

    // progress | complete |  keep_going
    // ==================================
    //    1     |    1     |      0
    //    1     |    0     |      1
    //    0     |    1     |      0
    //    0     |    0     |      0
    // ==================================
    keep_going &= (ret.modified && !is_complete);
  }

  return false;
}

int main(int argc, char *argv[]) {
  auto grid = std::make_unique<Grid>();

  if (grid->initialize()) {
    std::cout << "Invalid grid...\n";
    return 1;
  }

  for (int i = 1; i < argc; ++i) {
    const char *opt = argv[i];
    if (std::strcmp(opt, "-d") == 0 || std::strcmp(opt, "--debug") == 0) {
      DEBUG = true;
    } else if (std::strcmp(opt, "-t") == 0 || std::strcmp(opt, "--time") == 0) {
      TIME = true;
    } else if (std::strcmp(opt, "-p") == 0 ||
               std::strcmp(opt, "--print-after-all") == 0) {
      PRINT_AFTER_STEPS = true;
    } else if (std::strcmp(opt, "--no-colour") == 0) {
      USE_COLOUR = false;
    } else if (std::strcmp(opt, "-h") == 0 || std::strcmp(opt, "--help") == 0) {
      print_help();
      return 0;
    } else {
      std::cout << "Unrecognized argument '" << opt << "'...\n";
      print_help();
      return 1;
    }
  }

  std::cout << "Starting Out...\n";
  printGrid(grid.get(), USE_COLOUR);

  unsigned step_count = 0;
  bool is_complete = false;

  const bool has_error = solveGrid(grid.get(), is_complete, step_count);

  printGrid(grid.get(), USE_COLOUR);

  if (has_error) {
    std::cout << "Found a bad (invalid) grid!\n";
    return 1;
  } else if (is_complete) {
    std::cout << "Complete in " << step_count << " steps!\n";
  } else {
    std::cout << "Stuck after " << step_count << " steps!\n";
    return 1;
  }

  return 0;
}
