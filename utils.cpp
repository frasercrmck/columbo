#include "utils.h"

#include <sstream>

bool USE_ROWCOL = true;

CellCountMaskArray collectCellCountMaskInfo(const House &house) {
  CellCountMaskArray cell_masks{};
  for (const auto &cell : house) {
    const auto &candidates = cell->candidates;
    for (std::size_t i = 0; i < 9; ++i) {
      if (candidates[i]) {
        cell_masks[i] |= (1 << house.getLinearID(cell));
      }
    }
  }
  return cell_masks;
}

void printIntList(std::ostream &os, IntList const &list) {
  bool sep = false;
  os << "[";
  for (unsigned i = 0, e = list.size(); i != e; i++) {
    os << (sep ? "," : "") << (unsigned)list[i];
    sep = true;
  }
  os << "]";
}
