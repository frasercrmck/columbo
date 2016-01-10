#include "defs.h"

void Cage::addCells(Grid *const grid, std::initializer_list<Coord> coords) {
  for (auto &coord : coords) {
    cells.push_back(grid->getCell(coord));
  }
}
