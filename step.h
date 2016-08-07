#ifndef COLUMBO_STEP_H
#define COLUMBO_STEP_H

#include "defs.h"

#include <set>
#include <map>
#include <string>

struct Grid;
struct Cell;

// A return code as produced by a Step. Contains whether the Step produced an
// error, and whether it modified the grid in any way.
struct StepCode {
  bool error;
  bool modified;

  StepCode operator|=(const StepCode &other) {
    this->modified |= other.modified;
    this->error |= other.error;
    return *this;
  }

  operator bool() const { return this->error; }
};

struct ColumboStep {

  virtual ~ColumboStep();

  virtual void anchor() = 0;

  virtual const char *getID() const = 0;
  virtual const char *getName() const = 0;

  virtual StepCode runOnGrid(Grid *const grid) = 0;

  const CellSet &getChanged() const { return changed; }

protected:
  CellSet changed;
};

using StepIDMap = std::map<std::string, ColumboStep *>;

#endif // COLUMBO_STEP_H
