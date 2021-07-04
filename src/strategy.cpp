#include "strategy.h"
#include "printers/terminal_printer.h"

#include <iostream>
#include <chrono>
#include <algorithm>

bool TIME = false;
bool USE_COLOUR = true;

static bool checkIsGridComplete(Grid *const grid) {
  return std::all_of(std::begin(grid->rows), std::end(grid->rows),
                     [](const std::unique_ptr<House> &r) {
                       return std::all_of(
                           std::begin(*r), std::end(*r),
                           [](const Cell *c) { return c->isFixed(); });
                     });
}

// Remove permutations that hold a value that the given cell no longer
// considers a candidate.
static void trimPermutations(CageCombo &cage_combo, Mask candidate_mask,
                             unsigned cell_idx) {
  cage_combo.permutations.erase(
      std::remove_if(std::begin(cage_combo.permutations),
                     std::end(cage_combo.permutations),
                     [&candidate_mask, &cell_idx](IntList &list) {
                       return !candidate_mask[list[cell_idx] - 1];
                     }),
      std::end(cage_combo.permutations));
}

// Clean up impossible cage combinations after a step has modified the grid
static void cleanUpCageCombos(CellSet &changed) {
  for (auto *cell : changed) {
    for (auto *cage : cell->all_cages()) {
      const Mask mask = cell->candidates;

      unsigned cell_idx = *cage->indexOf(cell);

      if (!cage->cage_combos)
        continue;
        //throw invalid_grid_exception{"Cages must have combo information"};

      auto &cage_combos = *cage->cage_combos;
      // Remove any subsets that use a number that the cell no longer
      // considers a candidate.
      for (auto &cage_combo : cage_combos)
        trimPermutations(cage_combo, mask, cell_idx);

      // Remove any cage combos who have run out of permutations.
      cage_combos.combos.erase(
          std::remove_if(std::begin(cage_combos), std::end(cage_combos),
                         [](CageCombo const &cage_combo) {
                           return cage_combo.permutations.empty();
                         }),
          std::end(cage_combos));
    }
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
  bool modified = step->runOnGrid(grid, dbg_opts);

  if (TIME) {
    auto end = std::chrono::steady_clock::now();
    auto diff_ms =
        std::chrono::duration<double, std::milli>(end - start).count();
    std::cout << step->getName() << " took " << diff_ms << "ms...\n";
  }

  if (!modified)
    return modified;

  assert(!step->getChanged().empty() && "Expected 'modified' to change cells");

  auto changed = step->getChanged();
  cleanUpCageCombos(changed);

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
    for (int i = 0; i <= repeat_count.value_or(0); i++) {
      stats.modified = false;
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

        // If the step has made any modifications, start from the beginning.
        // This limits the amount of times we run expensive steps.
        if (stats.modified && repeat_count.has_value())
          break;
      }

      stats.is_complete = checkIsGridComplete(grid);

      if (!stats.modified || stats.is_complete) {
        return stats;
      }
    }

    return stats;
  }

  for (int i = 0; i <= repeat_count.value_or(0); i++) {
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
    std::cerr << "Could not add step '" << id << "'\n";
    return true;
  }
  steps.push_back(step_map[id]);
  return false;
}

bool Strategy::initializeDefault(StepIDMap &steps) {
  bool err = false;
  main_block = std::make_unique<Block>(100);

  err |= main_block->addStep("fixed-cell-cleanup", steps);
  err |= main_block->addStep("impossible-combos", steps);
  err |= main_block->addStep("conflicting-combos", steps);
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
  err |= main_block->addStep("naked-quints", steps);
  err |= main_block->addStep("innies-outies-hard", steps);
  err |= main_block->addStep("conflicting-combos-hard", steps);
  err |= main_block->addStep("cage-unit-overlap-hard", steps);

  return err;
}

bool Strategy::initializeWithSteps(const std::vector<std::string> &to_run,
                                   StepIDMap &steps) {
  main_block = std::make_unique<Block>();
  bool err = main_block->addStep("fixed-cell-cleanup", steps);
  for (const auto &step_id : to_run)
    err |= main_block->addStep(step_id.c_str(), steps);
  return err;
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
