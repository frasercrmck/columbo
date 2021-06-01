#include "strategy.h"
#include "printers/terminal_printer.h"

#include <iostream>
#include <chrono>
#include <algorithm>

bool DEBUG = false;
bool TIME = false;
bool USE_COLOUR = true;

static bool checkIsGridComplete(Grid *const grid) {
  for (auto &row : grid->rows) {
    for (auto *cell : *row.get()) {
      if (!cell->isFixed()) {
        return false;
      }
    }
  }
  return true;
}

// Clean up impossible cage combinations after a step has modified the grid
static void cleanUpCageCombos(CellSet &changed, CageSubsetMap &map) {
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
                                        return !mask[list[cell_idx] - 1];
                                      }),
                       cage_subsets.end());
  }
}

static bool runStep(Grid *grid, ColumboStep *step,
                    const DebugOptions &dbg_opts) {
  // Store the 'before' output to a stringstream as it's not very interesting
  // if the step does nothing.
  std::stringstream ss;
  if (dbg_opts.print_before_all ||
      dbg_opts.print_before_steps.count(step->getID())) {
    printGrid(grid, ss, USE_COLOUR, /*before*/ true, step->getName());
  }
  auto start = std::chrono::steady_clock::now();
  bool modified = step->runOnGrid(grid);

  if (TIME) {
    auto end = std::chrono::steady_clock::now();
    auto diff_ms =
        std::chrono::duration<double, std::milli>(end - start).count();
    std::cout << step->getName() << " took " << diff_ms << "ms...\n";
  }

  if (!modified) {
    if (DEBUG) {
      std::cout << step->getName() << " did nothing...\n";
    }
    return modified;
  }

  assert(!step->getChanged().empty() && "Expected 'modified' to change cells");

  auto changed = step->getChanged();
  cleanUpCageCombos(changed, *grid->getSubsetMap());

  if (dbg_opts.print_before_all ||
      dbg_opts.print_before_steps.count(step->getID())) {
    std::cout << ss.str();
  }

  if (dbg_opts.print_after_all ||
      dbg_opts.print_after_steps.count(step->getID())) {
    printGrid(grid, std::cout, USE_COLOUR, /*before*/ false, step->getName());
  }

  return modified;
}

Stats Block::runOnGrid(Grid *const grid, const DebugOptions &dbg_opts) {
  Stats stats;
  auto cleanup_step = std::make_unique<PropagateFixedCells>();

  if (blocks.empty()) {
    for (int i = 0; i < repeat; i++) {
      for (auto *step : steps) {
        stats.modified |= runStep(grid, step, dbg_opts);

        stats.num_steps++;

        if (step->getChanged().empty()) {
          continue;
        }

        stats.num_useful_steps++;

        // Do some fixed-cell cleanup
        cleanup_step->setWorkList(step->getChanged());
        stats.modified |= runStep(grid, cleanup_step.get(), dbg_opts);
      }

      stats.is_complete = checkIsGridComplete(grid);

      if (!stats.modified || stats.is_complete) {
        return stats;
      }
    }

    return stats;
  }

  for (int i = 0; i < repeat; i++) {
    for (auto &b : blocks) {
      stats |= b->runOnGrid(grid, dbg_opts);
    }

    stats.is_complete |= checkIsGridComplete(grid);

    if (!stats.modified || stats.is_complete) {
      return stats;
    }
  }

  return stats;
}

bool Block::addStep(const char *id, StepIDMap &step_map) {
  if (step_map.find(id) == step_map.end()) {
    std::cout << "Could not add step '" << id << "'\n";
    return true;
  }
  steps.push_back(step_map[id]);
  return false;
}

bool Strategy::initializeDefault(StepIDMap &steps) {
  bool err = false;
  main_block = std::make_unique<Block>(10);

  err |= main_block->addStep("fixed-cell-cleanup", steps);
  err |= main_block->addStep("impossible-combos", steps);
  err |= main_block->addStep("naked-pairs", steps);
  err |= main_block->addStep("naked-triples", steps);
  err |= main_block->addStep("hidden-singles", steps);
  err |= main_block->addStep("hidden-pairs", steps);
  err |= main_block->addStep("hidden-triples", steps);
  err |= main_block->addStep("hidden-quads", steps);
  err |= main_block->addStep("cage-unit-overlap", steps);
  err |= main_block->addStep("pointing-pairs-triples", steps);
  err |= main_block->addStep("innies-outies", steps);
  err |= main_block->addStep("x-wings", steps);
  err |= main_block->addStep("naked-quads", steps);

  return err;
}

bool Strategy::initializeSingleStep(const char *id, StepIDMap &steps) {
  main_block = std::make_unique<Block>(1);
  bool err = main_block->addStep("fixed-cell-cleanup", steps);
  return err | main_block->addStep(id, steps);
}

Stats Strategy::solveGrid(Grid *const grid, const DebugOptions &dbg_opts) {
  auto start = std::chrono::steady_clock::now();
  Stats stats = main_block->runOnGrid(grid, dbg_opts);
  if (TIME) {
    auto end = std::chrono::steady_clock::now();
    auto diff_ms =
        std::chrono::duration<double, std::milli>(end - start).count();
    std::cout << "Took " << diff_ms << "ms in total\n";
  }
  return stats;
}
