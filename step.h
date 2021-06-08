#ifndef COLUMBO_STEP_H
#define COLUMBO_STEP_H

#include "defs.h"

#include <set>
#include <map>
#include <string>
#include <optional>

struct Grid;
struct Cell;

struct invalid_grid_exception : public std::exception {
  explicit invalid_grid_exception() {}
  explicit invalid_grid_exception(std::string &&msg) : msg(msg) {}
  const char *what() const noexcept override { return msg.c_str(); }
  std::string msg = "invalid grid";
};

struct ColumboStep {

  virtual ~ColumboStep();

  virtual void anchor() = 0;

  virtual const char *getID() const = 0;
  virtual const char *getName() const = 0;

  // Updates a cell's candidates if there's anthing to update
  // Optionally returns the intersection upon success
  std::optional<Mask> updateCell(Cell *cell, const Mask mask) {
    const auto intersection = cell->candidates & ~mask;
    if (intersection.none() || intersection == cell->candidates) {
      return std::nullopt;
    }
    changed.insert(cell);
    cell->candidates &= mask;
    return intersection;
  }

  virtual bool runOnGrid(Grid *const grid) = 0;

  const CellSet &getChanged() const { return changed; }

protected:
  CellSet changed;
};

using StepIDMap = std::map<std::string, ColumboStep *>;

#endif // COLUMBO_STEP_H
