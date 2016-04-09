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

unsigned solveGrid(Grid *const grid, bool &has_error, bool &is_complete) {
  // Helper to facilitate printing of the grid after each step
  auto printGridIfNeeded = [&grid](const ColumboStep *step,
                                   const bool modified) {
    if (modified) {
      if (PRINT_AFTER_STEPS) {
        printGrid(grid, USE_COLOUR, step->getName());
      }
    } else if (DEBUG) {
      std::cout << step->getName() << " did nothing...\n";
    }
  };

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

  unsigned step_no = 0;
  bool keep_going = true;
  while (keep_going) {
    ++step_no;
    bool progress = false;
    for (auto &step : steps) {
      auto start = std::chrono::steady_clock::now();
      StepCode ret = step->runOnGrid(grid);

      if (ret) {
        // Detected an error (invalid grid)
        has_error = true;
        return step_no;
      }

      if (TIME) {
        auto end = std::chrono::steady_clock::now();
        auto diff_ms =
            std::chrono::duration<double, std::milli>(end - start).count();
        std::cout << step->getName() << " took " << diff_ms << "ms...\n";
      }

      printGridIfNeeded(step.get(), ret.modified);

      auto changed = step->getChanged();

      if (!changed.empty()) {
        bool cleaned_up = false;
        cleanup_step->setWorkList(changed);
        while (cleanup_step->runOnGrid(grid).modified) {
          cleaned_up = true;
          cleanup_step->setWorkList(cleanup_step->getChanged());
        }

        printGridIfNeeded(cleanup_step.get(), cleaned_up);

        ret.modified |= cleaned_up;
      }

      progress |= ret.modified;
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
    keep_going &= (progress && !is_complete);
  }

  return step_no;
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

  bool has_error = false;
  bool is_complete = false;

  const auto step_count = solveGrid(grid.get(), has_error, is_complete);

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
