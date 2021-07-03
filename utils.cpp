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

Printable printIntList(IntList const &list) {
  return Printable([list](std::ostream &os) {
    bool sep = false;
    os << "[";
    for (unsigned i = 0, e = list.size(); i != e; i++) {
      os << (sep ? "," : "") << (unsigned)list[i];
      sep = true;
    }
    os << "]";
  });
}

Printable
printAnnotatedIntList(IntList const &list,
                      std::unordered_map<unsigned, char> const &symbol_map) {
  return Printable([list, symbol_map](std::ostream &os) {
    bool sep = false;
    os << "[";
    for (unsigned i = 0, e = list.size(); i != e; i++) {
      os << (sep ? "," : "") << (unsigned)list[i];
      if (auto it = symbol_map.find(i); it != symbol_map.end())
        os << it->second;
      sep = true;
    }
    os << "]";
  });
}
