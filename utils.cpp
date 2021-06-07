#include "utils.h"

#include <sstream>

std::string getHousePrintNum(House &house) {
  switch (house.getKind()) {
  case HouseKind::Row:
    return getID(house.num);
  case HouseKind::Col:
  case HouseKind::Box: {
    std::stringstream ss;
    ss << house.num;
    return ss.str();
  }
  }
}

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
