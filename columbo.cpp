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

bool solveGrid(Grid *const grid, bool &is_complete, unsigned &step_no) {
  CageSubsetMap *subset_map = grid->getSubsetMap();

  std::vector<std::unique_ptr<ColumboStep>> steps;
  steps.push_back(std::make_unique<EliminateImpossibleCombosStep>(subset_map));
  steps.push_back(std::make_unique<EliminateNakedPairsStep>());
  steps.push_back(std::make_unique<EliminateNakedTriplesStep>());
  steps.push_back(std::make_unique<EliminateHiddenSinglesStep>());
  steps.push_back(std::make_unique<EliminateHiddenPairsStep>());
  steps.push_back(std::make_unique<EliminateHiddenTriplesStep>());
  steps.push_back(std::make_unique<EliminateCageUnitOverlapStep>(subset_map));
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

      cleanUpCageCombos(changed, *subset_map);

      // Do some fixed-cell cleanup
      cleanup_step->setWorkList(changed);
      ret |= runStep(grid, cleanup_step.get());

      if (ret) {
        return true;
      }

      auto cleanup_changed = cleanup_step->getChanged();
      if (!cleanup_changed.empty()) {
        cleanUpCageCombos(cleanup_changed, *subset_map);
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
