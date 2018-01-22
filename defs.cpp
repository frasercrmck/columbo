#include "defs.h"
#include "init.h"

#include <set>
#include <iomanip>

const char *invalid_grid_exception::what() const noexcept {
  return "invalid grid";
}

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
      Cage *cage = grid->getCell(row, col)->cage;
      if (!visited_cages.insert(cage).second) {
        continue;
      }
      InnieOutie innie_outie;
      innie_outie.sum = cage->sum;

      // Collect cells found inside and outside the cage
      for (auto &cage_cell : *cage) {
        const Coord &coord = cage_cell->coord;
        if (coord.row >= min.row && coord.row <= max.row &&
            coord.col >= min.col && coord.col <= max.col) {
          innie_outie.cell_cage.cells.push_back(cage_cell);
        } else {
          innie_outie.unknown_cage.cells.push_back(cage_cell);
        }
      }

      if (innie_outie.cell_cage.size() == cage->size()) {
        // Add to the known total if all cells are inside
        for (auto *innie_cell : innie_outie.cell_cage) {
          known_cage.cells.push_back(innie_cell);
        }
        known_cage.sum += cage->sum;
        continue;
      }

      const auto num_innies = innie_outie.cell_cage.size();
      const auto num_outies = innie_outie.unknown_cage.size();

      if (num_innies == 1 || (num_outies != 1 && num_innies <= num_outies)) {
        innies.push_back(innie_outie);
      } else {
        std::swap(innie_outie.cell_cage, innie_outie.unknown_cage);
        outies.push_back(innie_outie);
      }
    }
  }
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

void Grid::writeToFile(std::ofstream &file) {
  for (auto &row : rows) {
    for (auto *c : *row) {
      file << "0x" << std::hex << std::setfill('0') << std::setw(3)
           << c->candidates.to_ulong() << ' ';
    }
    file << "\n";
  }

  file << "\n";
  file << std::dec;

  for (auto &cage : cages) {
    file << std::setfill(' ') << std::setw(2) << std::left << cage.sum;
    for (auto *cell : cage.cells) {
      file << ' ' << cell->coord;
    }
    file << "\n";
  }
  file.flush();
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
    generateSubsetSums(cage.sum, possibles, Duplicates::No, subsets);

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
