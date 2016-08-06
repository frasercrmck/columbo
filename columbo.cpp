#include "all_steps.h"
#include "defs.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <memory>

bool DEBUG = false;
static bool USE_COLOUR = true;
static bool PRINT_AFTER_STEPS = false;
static bool TIME = false;
static bool QUIET = false;

static void print_help() {
  std::cout << R"(
    COLUMBO: A solver of killers. Of killer sudokus.

    Usage:
      columbo [option] -f <sudoku file>

    Options:
      -h                         Print help and exit
      -f    --file <sudoku file> Use <sudoku file> as input
      -o           <sudoku file> Write <sudoku file> as output
      -p    --print-after-all    Print grid after every step
      -t    --time               Print detailed timing information
      -d    --debug              Print debug text for every step
      -s    --run-step <step>    Run <step> and only <step>
      -q    --quiet              Print nothing at all
            --no-colour          Don't print grids using colour

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

// Clean up impossible cage combinations after a step has modified the grid
void cleanUpCageCombos(CellSet &changed, CageSubsetMap &map) {
  for (auto *cell : changed) {
    Cage *cage = cell->cage;

    std::vector<IntList> &cage_subsets = map[cage];

    const Mask mask = cell->candidates.to_ulong();

    // Find the cell's id inside the cage
    // TODO: More efficient way of doing this?
    unsigned cell_idx = 0;
    for (auto *c : *cage) {
      if (c == cell) {
        break;
      }
      ++cell_idx;
    }

    // Remove any subsets that use a number that the cell no longer considers a
    // candidate.
    cage_subsets.erase(std::remove_if(cage_subsets.begin(), cage_subsets.end(),
                                      [&mask, &cell_idx](IntList &list) {
                                        return !isOn(mask, list[cell_idx] - 1);
                                      }),
                       cage_subsets.end());
  }
}

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

  if (!ret.modified) {
    return ret;
  }

  assert(!step->getChanged().empty() && "Expected 'modified' to change cells");

  auto changed = step->getChanged();
  cleanUpCageCombos(changed, *grid->getSubsetMap());

  printGridIfNeeded(grid, step, ret.modified);

  return ret;
}

using StepList = std::vector<std::unique_ptr<ColumboStep>>;

static void initializeAllSteps(const Grid *grid, StepList &steps) {
  CageSubsetMap *subset_map = grid->getSubsetMap();

  steps.push_back(std::make_unique<EliminateImpossibleCombosStep>(subset_map));
  steps.push_back(std::make_unique<EliminateNakedPairsStep>());
  steps.push_back(std::make_unique<EliminateNakedTriplesStep>());
  steps.push_back(std::make_unique<EliminateHiddenSinglesStep>());
  steps.push_back(std::make_unique<EliminateHiddenPairsStep>());
  steps.push_back(std::make_unique<EliminateHiddenTriplesStep>());
  steps.push_back(std::make_unique<EliminateCageUnitOverlapStep>(subset_map));
  steps.push_back(std::make_unique<EliminatePointingPairsOrTriplesStep>());
  steps.push_back(std::make_unique<EliminateOneCellInniesAndOutiesStep>());
}

bool solveGrid(Grid *const grid, bool &is_complete, unsigned &step_no) {
  StepList steps;
  initializeAllSteps(grid, steps);

  auto cleanup_step = std::make_unique<PropagateFixedCells>();

  bool keep_going = true;
  while (keep_going) {
    ++step_no;
    StepCode ret = {false, false};
    for (auto &step : steps) {
      auto local_ret = runStep(grid, step.get());

      ret |= local_ret;

      if (ret) {
        return true;
      } else if (!local_ret.modified) {
        continue;
      }

      // Do some fixed-cell cleanup
      cleanup_step->setWorkList(step->getChanged());
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

static bool isOpt(const char *arg, const char *flag, const char *name) {
  return std::strcmp(arg, flag) == 0 || std::strcmp(arg, name) == 0;
}

int main(int argc, char *argv[]) {
  const char *file_name = nullptr;
  const char *out_file_name = nullptr;
  const char *step_to_run = nullptr;

  for (int i = 1; i < argc; ++i) {
    const char *opt = argv[i];
    if (isOpt(opt, "-d", "--debug")) {
      DEBUG = true;
    } else if (isOpt(opt, "-t", "--time")) {
      TIME = true;
    } else if (isOpt(opt, "-p", "--print-after-all")) {
      PRINT_AFTER_STEPS = true;
    } else if (isOpt(opt, "", "--no-colour")) {
      USE_COLOUR = false;
    } else if (isOpt(opt, "-f", "--file")) {
      if (i + 1 >= argc) {
        std::cout << "Expected a value to option '" << opt << "'...\n";
        return 1;
      }
      file_name = argv[i + 1];
      ++i;
    } else if (isOpt(opt, "-o", "")) {
      if (i + 1 >= argc) {
        std::cout << "Expected a value to option '" << opt << "'...\n";
        return 1;
      }
      out_file_name = argv[i + 1];
      ++i;
    } else if (isOpt(opt, "-s", "--run-step")) {
      if (i + 1 >= argc) {
        std::cout << "Expected a value to option '" << opt << "'...\n";
        return 1;
      }
      step_to_run = argv[i + 1];
      ++i;
    } else if (isOpt(opt, "-h", "--help")) {
      print_help();
      return 0;
    } else if (isOpt(opt, "-q", "--quiet")) {
      QUIET = true;
    } else {
      std::cout << "Unrecognized argument '" << opt << "'...\n";
      print_help();
      return 1;
    }
  }

  if (!file_name) {
    std::cout << "Did not specify a file name\n";
    print_help();
    return 1;
  }

  if (QUIET) {
    DEBUG = false;
    PRINT_AFTER_STEPS = false;
  }

  std::ifstream sudoku_file;
  sudoku_file.open(file_name);

  if (!sudoku_file.is_open()) {
    std::cout << "Could not open file '" << file_name << "'...\n";
    return 1;
  }

  auto grid = std::make_unique<Grid>();

  if (grid->initialize(sudoku_file)) {
    std::cout << "Invalid grid...\n";
    return 1;
  }

  sudoku_file.close();

  if (!QUIET) {
    std::cout << "Starting Out...\n";
    printGrid(grid.get(), USE_COLOUR);
  }

  if (step_to_run) {
    StepList steps;
    initializeAllSteps(grid.get(), steps);
    for (auto &step : steps) {
      if (std::strcmp(step->getID(), step_to_run) != 0) {
        continue;
      }
      bool err = runStep(grid.get(), step.get());
      if (err) {
        std::cout << "Found a bad grid!\n";
      }
      return err == 1;
    }
    std::cout << "Could not find step to run: '" << step_to_run << "'\n";
    return 1;
  }

  unsigned step_count = 0;
  bool is_complete = false;

  const bool has_error = solveGrid(grid.get(), is_complete, step_count);

  if (!QUIET) {
    printGrid(grid.get(), USE_COLOUR);
  }

  if (out_file_name) {
    std::ofstream out_file;
    out_file.open(out_file_name);

    grid->writeToFile(out_file);
  }

  if (has_error) {
    std::cout << "Found a bad (invalid) grid!\n";
    return 1;
  } else if (is_complete) {
    if (!QUIET) {
      std::cout << "Complete in " << step_count << " steps!\n";
    }
  } else {
    std::cout << "Stuck after " << step_count << " steps!\n";
    return 1;
  }

  return 0;
}
