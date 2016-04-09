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

  bool has_error = false;
  bool is_complete = false;

  int step_no = 0;
  bool keep_going = true;
  for (; keep_going; ++step_no) {
    bool progress = false;
    for (auto &step : steps) {
      auto start = std::chrono::steady_clock::now();
      const StepCode ret = step->runOnGrid(grid.get());

      if (ret) {
        has_error = true;
        keep_going = false;
        break;
      }

      if (TIME) {
        auto end = std::chrono::steady_clock::now();
        auto diff_ms =
            std::chrono::duration<double, std::milli>(end - start).count();
        std::cout << step->getName() << " took " << diff_ms << "ms...\n";
      }

      if (ret.modified) {
        if (PRINT_AFTER_STEPS) {
          printGrid(grid.get(), USE_COLOUR, step->getName());
        }
      } else if (DEBUG) {
        std::cout << step->getName() << " did nothing...\n";
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

  printGrid(grid.get(), USE_COLOUR);

  if (has_error) {
    std::cout << "Found a bad (invalid) grid!\n";
  } else if (is_complete) {
    std::cout << "Complete in " << step_no << " steps!\n";
  } else {
    std::cout << "Stuck after " << step_no << " steps!\n";
    return 1;
  }

  return 0;
}
