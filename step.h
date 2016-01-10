#ifndef COLUMBO_STEP_H
#define COLUMBO_STEP_H

struct Grid;

struct ColumboStep {

  virtual ~ColumboStep();

  virtual void anchor() = 0;

  virtual const char *getName() const = 0;

  virtual bool runOnGrid(Grid *const grid) = 0;
};

#endif // COLUMBO_STEP_H
