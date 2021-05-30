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
          innie_outie.inside_cage.cells.push_back(cage_cell);
        } else {
          innie_outie.outside_cage.cells.push_back(cage_cell);
        }
      }

      if (innie_outie.inside_cage.size() == cage->size()) {
        // Add to the known total if all cells are inside
        for (auto *innie_cell : innie_outie.inside_cage) {
          known_cage.cells.push_back(innie_cell);
        }
        known_cage.sum += cage->sum;
        continue;
      }

      innies_outies.push_back(innie_outie);
    }
  }
}

Cell *Grid::getCell(const Coord &coord) {
  return getCell(coord.row, coord.col);
}

Cell *Grid::getCell(unsigned y, unsigned x) { return &(cells[y][x]); }

bool Grid::initialize(std::ifstream &file, bool v) {
  if (initializeGridFromFile(file, this))
    return true;
  if (initializeCages())
    return true;
  if (v && validate())
    return true;
  assignCageColours();
  initializeCageSubsetMap();
  initializeInnieAndOutieRegions();
  return false;
}

bool Grid::initializeCages() {
  for (auto &cage : cages) {
    for (auto &cell : cage->cells)
      cell->cage = cage.get();
  }

  std::array<bool, 81> seen_cells;
  std::fill(seen_cells.begin(), seen_cells.end(), false);

  bool invalid = false;
  for (auto &cage : cages) {
    for (auto &cell : cage->cells) {
      const unsigned idx = cell->coord.row * 9 + cell->coord.col;
      if (seen_cells[idx]) {
        invalid = true;
        std::cout << "Duplicated cell " << cell->coord << "\n";
      }
      seen_cells[idx] = true;
    }
  }

  for (unsigned row = 0; row < 9; ++row) {
    for (unsigned col = 0; col < 9; ++col) {
      auto *cell = getCell(row, col);
      if (cell->cage) {
        continue;
      }
      invalid = true;
      std::cout << "No cage for " << cell->coord << "\n";
    }
  }

  return invalid;
}

bool Grid::validate() {
  unsigned total_sum = 0;
  for (auto &cage : cages)
    total_sum += cage->sum;

  if (total_sum != 405) {
    std::cout << "Error: cage total (" << total_sum << ") is not 405\n";
    return true;
  }
  return false;
  ;
}

void Grid::assignCageColours() {
  for (auto &cage : cages) {
    cage->colour = 0;
  }
  // Create the cage graph: edges represent neighbours
  std::map<Cage *, std::set<Cage *>> cage_graph;
  for (unsigned row = 0; row < 9; ++row) {
    for (unsigned col = 0; col < 9; ++col) {
      auto *cell = getCell(row, col);
      auto *right_cell = col != 8 ? getCell(row, col + 1) : nullptr;
      if (right_cell && cell->cage != right_cell->cage) {
        cage_graph[cell->cage].insert(right_cell->cage);
        cage_graph[right_cell->cage].insert(cell->cage);
      }
      auto *down_cell = row != 8 ? getCell(row + 1, col) : nullptr;
      if (down_cell && cell->cage != down_cell->cage) {
        cage_graph[cell->cage].insert(down_cell->cage);
        cage_graph[down_cell->cage].insert(cell->cage);
      }
    }
  }

  // Allocate colours to cages. Greedy graph colouring. Colour 0 is effectively
  // 'unassigned'.
  for (auto &cage : cages) {
    // Build up a bitmask of used colours
    Mask used_mask = 0;
    for (auto &neighbour : cage_graph[cage.get()]) {
      if (neighbour->colour) {
        used_mask |= (1 << (neighbour->colour - 1));
      }
    }
    // Now search for first unused colour
    unsigned i = 0;
    while (used_mask[i]) {
      ++i;
    }
    cage->colour = static_cast<int>(i + 1);
  }
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
    file << std::setfill(' ') << std::setw(2) << std::left << cage->sum;
    for (auto *cell : cage->cells) {
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
    possibles.resize(cage->size());

    unsigned idx = 0;
    for (auto &cell : cage->cells) {
      for (unsigned x = 0; x < 9; ++x) {
        if (cell->candidates[x]) {
          possibles[idx].push_back(x + 1);
        }
      }
      ++idx;
    }

    std::vector<IntList> subsets;
    generateSubsetSums(cage->sum, possibles, Duplicates::No, subsets);

    (*subset_map)[cage.get()] = std::move(subsets);
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

std::ostream &operator<<(std::ostream &os, const Coord &coord) {
  os << getID(coord.row) << coord.col;
  return os;
}
