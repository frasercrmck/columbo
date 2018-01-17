#include "strategy.h"

#include <iostream>

bool DEBUG = false;
bool TIME = false;
bool PRINT_AFTER_STEPS = false;
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

static StepCode runStep(Grid *grid, ColumboStep *step) {
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
    if (DEBUG) {
      std::cout << step->getName() << " did nothing...\n";
    }
    return ret;
  }

  assert(!step->getChanged().empty() && "Expected 'modified' to change cells");

  auto changed = step->getChanged();
  cleanUpCageCombos(changed, *grid->getSubsetMap());

  if (PRINT_AFTER_STEPS) {
    printGrid(grid, USE_COLOUR, step->getName());
  }

  return ret;
}

StepCode Block::runOnGrid(Grid *const grid, Stats &stats) {
  StepCode ret = {false, false};

  auto cleanup_step = std::make_unique<PropagateFixedCells>();

  if (blocks.empty()) {
    for (int i = 0; i < repeat; i++) {
      for (auto *step : steps) {
        ret |= runStep(grid, step);

        stats.num_steps++;

        if (ret) {
          return ret;
        } else if (step->getChanged().empty()) {
          continue;
        }

        stats.num_useful_steps++;

        // Do some fixed-cell cleanup
        cleanup_step->setWorkList(step->getChanged());
        ret |= runStep(grid, cleanup_step.get());

        if (ret) {
          return ret;
        }
      }

      stats.is_complete = checkIsGridComplete(grid);

      if (!ret.modified || stats.is_complete) {
        return ret;
      }
    }

    return ret;
  }

  for (int i = 0; i < repeat; i++) {
    for (auto &b : blocks) {
      ret |= b->runOnGrid(grid, stats);
      if (ret) {
        return ret;
      }
    }

    stats.is_complete = checkIsGridComplete(grid);

    if (!ret.modified || stats.is_complete) {
      return ret;
    }
  }

  return ret;
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
  err |= main_block->addStep("cage-unit-overlap", steps);
  err |= main_block->addStep("pointing-pairs-triples", steps);
  err |= main_block->addStep("innies-outies", steps);

  return err;
}

bool Strategy::initializeSingleStep(const char *id, StepIDMap &steps) {
  main_block = std::make_unique<Block>(1);
  bool err = main_block->addStep("fixed-cell-cleanup", steps);
  return err | main_block->addStep(id, steps);
}

bool Strategy::solveGrid(Grid *const grid, Stats &stats) {
  StepCode ret = main_block->runOnGrid(grid, stats);
  return ret.error;
}