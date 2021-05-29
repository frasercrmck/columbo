#include "defs.h"

void Cage::addCell(Grid *const grid, Coord coord) {
  cells.push_back(grid->getCell(coord));
}

void Cage::addCells(Grid *const grid, std::initializer_list<Coord> coords) {
  for (auto &coord : coords) {
    cells.push_back(grid->getCell(coord));
  }
}
