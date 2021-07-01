#ifndef COLUMBO_UTILS_H
#define COLUMBO_UTILS_H

#include "defs.h"

extern bool USE_ROWCOL;

static inline const char *getRowID(unsigned id, bool use_rowcol) {
  switch (id) {
  default:
    return "X";
  case A:
    return use_rowcol ? "1" : "A";
  case B:
    return use_rowcol ? "2" : "B";
  case C:
    return use_rowcol ? "3" : "C";
  case D:
    return use_rowcol ? "4" : "D";
  case E:
    return use_rowcol ? "5" : "E";
  case F:
    return use_rowcol ? "6" : "F";
  case G:
    return use_rowcol ? "7" : "G";
  case H:
    return use_rowcol ? "8" : "H";
  case J:
    return use_rowcol ? "9" : "J";
  }
}

using CellCountMaskArray = std::array<Mask, 9>;

CellCountMaskArray collectCellCountMaskInfo(const House &house);

void printIntList(std::ostream &os, IntList const &list);

#endif // COLUMBO_UTILS_H
