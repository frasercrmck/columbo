#ifndef COLUMBO_FIXED_CELL_CLEANUP_H
#define COLUMBO_FIXED_CELL_CLEANUP_H

#include "debug.h"
#include "defs.h"
#include "step.h"

struct PropagateFixedCells : ColumboStep {
  StepCode runOnGrid(Grid *const grid) override {
    changed.clear();
    StepCode ret = {false, false};

    if (work_list.empty()) {
      for (auto &r : grid->cells) {
        for (auto &c : r) {
          if (c.isFixed()) {
            work_list.insert(&c);
          }
        }
      }
    }

    while (!work_list.empty()) {
      Cell *cell = *work_list.begin();
      work_list.erase(work_list.begin());

      if (!cell->isFixed()) {
        continue;
      }

      const auto col_id = cell->coord.col;
      const auto row_id = cell->coord.row;
      const auto box_id = (col_id / 3) + (row_id / 3) * 3;
      auto *row = grid->rows[row_id].get();
      auto *col = grid->cols[col_id].get();
      auto *box = grid->boxes[box_id].get();

      ret |= runOnHouse(*row, cell);
      ret |= runOnHouse(*col, cell);
      ret |= runOnHouse(*box, cell);
    }
    return ret;
  }

  virtual void anchor() override;

  const char *getID() const override { return "fixed-cell-cleanup"; }
  const char *getName() const override { return "Cleaning Up"; }

  void setWorkList(const CellSet &cells) { work_list = cells; }

private:
  CellSet work_list;
  StepCode runOnHouse(House &house, const Cell *cell);
};

#endif // COLUMBO_FIXED_CELL_CLEANUP_H
