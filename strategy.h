#ifndef COLUMBO_STRATEGY_H
#define COLUMBO_STRATEGY_H

#include <vector>
#include <memory>
#include <cassert>

#include "defs.h"
#include "step.h"
#include "utils.h"
#include "fixed_cell_cleanup.h"

extern bool DEBUG;
extern bool TIME;
extern bool PRINT_AFTER_STEPS;
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

struct Block {
  int repeat = 1;
  std::vector<ColumboStep*> steps;
  std::vector<std::unique_ptr<Block>> blocks;

  Block(int r) : repeat(r) {}

  bool addStep(const char *id, StepIDMap &step_map);

  Stats runOnGrid(Grid *const grid);
};

struct Strategy {
  std::unique_ptr<Block> main_block;

  bool initializeDefault(StepIDMap &steps);
  bool initializeSingleStep(const char *id, StepIDMap &steps);

  Stats solveGrid(Grid *const grid);
};

#endif // COLUMBO_STRATEGY_H
