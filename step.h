#ifndef COLUMBO_STEP_H
#define COLUMBO_STEP_H

struct Grid;

// A return code as produced by a Step. Contains whether the Step produced an
// error, and whether it modified the grid in any way.
struct StepCode {
  bool error;
  bool modified;

  StepCode operator|=(const StepCode &other) {
    this->modified |= other.modified;
    this->error = other.error ? other.error : this->error;
    return *this;
  }

  operator bool() const { return this->error; }
};

struct ColumboStep {

  virtual ~ColumboStep();

  virtual void anchor() = 0;

  virtual const char *getName() const = 0;

  virtual StepCode runOnGrid(Grid *const grid) = 0;
};

#endif // COLUMBO_STEP_H
