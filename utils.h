#ifndef COLUMBO_UTILS_H
#define COLUMBO_UTILS_H

#include "defs.h"

static inline const char *getID(unsigned id) {
  switch (id) {
  default:
    return "X";
  case A:
    return "A";
  case B:
    return "B";
  case C:
    return "C";
  case D:
    return "D";
  case E:
    return "E";
  case F:
    return "F";
  case G:
    return "G";
  case H:
    return "H";
  case J:
    return "J";
  }
}

std::string getHousePrintNum(House &house);

using CellCountMaskArray = std::array<Mask, 9>;

CellCountMaskArray collectCellCountMaskInfo(const House &house);

#endif // COLUMBO_UTILS_H
