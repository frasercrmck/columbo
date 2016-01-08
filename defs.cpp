#include "defs.h"
#include "init.h"

#include <set>

House::~House() {}

unsigned Row::getLinearID(const Cell *const cell) const {
  return cell->coord.col;
}

unsigned Col::getLinearID(const Cell *const cell) const {
  return cell->coord.row;
}

unsigned Box::getLinearID(const Cell *const cell) const {
  return (cell->coord.col % 3) + (3 * (cell->coord.row % 3));
}

void InnieOutieRegion::initialize(Grid *const grid) {
  std::set<Cage *> visited_cages;
  for (unsigned row = min_house.row; row <= max_house.row; ++row) {
    for (unsigned col = min_house.col; col <= max_house.col; ++col) {
      Cell *cell = grid->getCell(row, col);
      if (visited_cages.count(cell->cage)) {
        continue;
      }
      std::set<Cell *> cells_inside;
      std::set<Cell *> cells_outside;
      // Collect cells found inside and outside the cage
      for (auto &cage_cell : *cell->cage) {
        const Coord &coord = cage_cell->coord;
        if (coord.row >= min_house.row && coord.row <= max_house.row &&
            coord.col >= min_house.col && coord.col <= max_house.col) {
          cells_inside.insert(cage_cell);
        } else {
          cells_outside.insert(cage_cell);
        }
      }

      bool will_add_outie = cells_outside.size() == 1;

      if (cells_inside.size() == 1) {
        // We don't care about an outie in a 2-cell cage with one innie. The
        // outie will be solved automatically.
        will_add_outie &= cell->cage->cells.size() != 2;
        innie_cage.cells.push_back(*cells_inside.begin());
      } else if (cells_inside.size() == cell->cage->cells.size()) {
        // Add to the known total if all cells are inside
        for (auto &inside_cell : cells_inside) {
          known_cage.cells.push_back(inside_cell);
        }
        known_cage.sum += cell->cage->sum;
      } else {
        // Unknowns aren't very interesting if they're part of a cage
        // featuring an outie. They don't need to be known to solve an outie.
        // Once the outie is solved, we can mark them 'known'.
        if (!will_add_outie) {
          for (auto &inside_cell : cells_inside) {
            unknown_cage.cells.push_back(inside_cell);
          }
        }
      }
      if (will_add_outie) {
        outie_cage.cells.push_back(*cells_outside.begin());
      }
      visited_cages.insert(cell->cage);
    }
  }

  if (innie_cage.cells.empty()) {
    unknown_cage.sum = expected_sum - known_cage.sum;
  }
  innie_cage.sum = 0;
  if (unknown_cage.cells.empty()) {
    innie_cage.sum = expected_sum - known_cage.sum;
  }
  outie_cage.sum = 0;
}

Cell *Grid::getCell(const Coord &coord) {
  return getCell(coord.row, coord.col);
}

Cell *Grid::getCell(unsigned y, unsigned x) {
  return &(cells[y][x]);
}

void Grid::initializeCages() {
  initCages(this);
}

void Grid::initializeInnieAndOutieRegions() {
  // Do column-oriented regions
  const int max_width = 3;
  for (unsigned width = 1; width <= max_width; ++width) {
    for (unsigned col = 0; col <= 9 - width; ++col) {
      auto region = std::make_unique<InnieOutieRegion>(
          Coord{0, col}, Coord{8, col + width - 1});
      region->initialize(this);
      if (region->known_cage.sum != region->expected_sum) {
        innies_and_outies.push_back(std::move(region));
      }
    }
  }

  // Do row-oriented regions
  for (unsigned width = 1; width <= max_width; ++width) {
    for (unsigned row = 0; row <= 9 - width; ++row) {
      auto region = std::make_unique<InnieOutieRegion>(
          Coord{row, 0}, Coord{row + width - 1, 8});
      region->initialize(this);
      if (region->known_cage.sum != region->expected_sum) {
        innies_and_outies.push_back(std::move(region));
      }
    }
  }
}
