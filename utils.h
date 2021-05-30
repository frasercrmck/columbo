#ifndef COLUMBO_UTILS_H
#define COLUMBO_UTILS_H

#include "defs.h"

enum class Duplicates { No, Yes };

void generateSubsetSums(const unsigned target_sum,
                        const std::vector<IntList> &possibles,
                        const Duplicates allow_duplicates,
                        std::vector<IntList> &subsets);

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
