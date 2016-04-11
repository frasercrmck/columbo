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
  for (unsigned row = min.row; row <= max.row; ++row) {
    for (unsigned col = min.col; col <= max.col; ++col) {
      Cell *cell = grid->getCell(row, col);
      if (visited_cages.count(cell->cage)) {
        continue;
      }
      CellSet cells_inside;
      CellSet cells_outside;
      // Collect cells found inside and outside the cage
      for (auto &cage_cell : *cell->cage) {
        const Coord &coord = cage_cell->coord;
        if (coord.row >= min.row && coord.row <= max.row &&
            coord.col >= min.col && coord.col <= max.col) {
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

Cell *Grid::getCell(unsigned y, unsigned x) { return &(cells[y][x]); }

bool Grid::initialize(std::ifstream &file) {
  if (initializeGridFromFile(file, this)) {
    return true;
  }
  if (initializeCages(this)) {
    return true;
  }
  initializeCageSubsetMap();
  initializeInnieAndOutieRegions();
  return false;
}

void Grid::initializeCageSubsetMap() {
  subset_map = std::make_unique<CageSubsetMap>();

  for (auto &cage : cages) {
    std::vector<IntList> possibles;
    possibles.resize(cage.size());

    unsigned idx = 0;
    for (auto &cell : cage.cells) {
      for (unsigned x = 0; x < 9; ++x) {
        if (cell->candidates[x]) {
          possibles[idx].push_back(x + 1);
        }
      }
      ++idx;
    }

    std::vector<IntList> subsets;
    generateSubsetSums(cage.sum, possibles, subsets);

    (*subset_map)[&cage] = std::move(subsets);
  }
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

  // Do block-oriented regions; only simple ones for now
  for (unsigned y = 0; y < 3; ++y) {
    for (unsigned x = 0; x < 3; ++x) {
      auto region = std::make_unique<InnieOutieRegion>(
          Coord{y * 3, x * 3}, Coord{y * 3 + 2, x * 3 + 2});
      region->initialize(this);
      if (region->known_cage.sum != region->expected_sum) {
        innies_and_outies.push_back(std::move(region));
      }
    }
  }
}
