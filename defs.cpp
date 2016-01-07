#include "defs.h"

House::~House() {}

unsigned Row::getLinearID(const Cell *const cell) const {
  return cell->coord.col;
}

unsigned Col::getLinearID(const Cell *const cell) const {
  return cell->coord.row;
}

unsigned Box::getLinearID(const Cell *const cell) const {
  return (cell->coord.col % 3) + (3 * (cell->coord.row % 3));
}
