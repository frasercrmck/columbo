#ifndef COLUMBO_STRATEGY_H
#define COLUMBO_STRATEGY_H

#include <cassert>
#include <memory>
#include <unordered_set>
#include <vector>

#include "defs.h"
#include "step.h"
#include "utils.h"
#include "fixed_cell_cleanup.h"

extern bool DEBUG;
extern bool TIME;
extern bool USE_COLOUR;

struct Stats {
  unsigned num_steps = 0;
  unsigned num_useful_steps = 0;

  bool modified = false;
  bool is_complete = false;

  Stats operator|=(const Stats &other) {
    num_steps += other.num_steps;
    num_useful_steps += other.num_useful_steps;
    modified |= other.modified;
    is_complete |= other.is_complete;
    return *this;
  }
};

struct DebugOptions {
  bool print_after_all = false;
  bool print_before_all = false;
  std::unordered_set<std::string> print_after_steps;
  std::unordered_set<std::string> print_before_steps;
};

struct Block {
  int repeat_count = 1;
  std::vector<ColumboStep*> steps;
  std::vector<std::unique_ptr<Block>> blocks;

  Block(int r) : repeat_count(r) {}

  bool addStep(const char *id, StepIDMap &step_map);

  Stats runOnGrid(Grid *const grid, const DebugOptions &dbg_opts);
};

struct Strategy {
  std::unique_ptr<Block> main_block;

  bool initializeDefault(StepIDMap &steps);
  bool initializeWithSteps(const std::vector<std::string> &to_run,
                           StepIDMap &steps);

  Stats solveGrid(Grid *const grid, const DebugOptions &dbg_opts);
};

#endif // COLUMBO_STRATEGY_H
