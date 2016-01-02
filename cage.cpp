#include "defs.h"
#include "utils.h"

#include <iostream>

void Cage::addCells(Grid *const grid, std::initializer_list<Coord> coords) {
  for (auto &coord : coords) {
    cells.push_back(getCell(grid, coord));
  }
}
