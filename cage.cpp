#include "defs.h"
#include "utils.h"

#include <iostream>

void Cage::addCells(Grid *const grid, std::initializer_list<Coord> coords) {
  for (auto &coord : coords) {
    auto *cell = getCell(grid, coord);
    if (cell->cage) {
      std::cout << "Clash: {" << coord.row << "," << coord.col << "}\n";
    }
    cells.push_back(cell);
    cell->cage = this;
  }
}
