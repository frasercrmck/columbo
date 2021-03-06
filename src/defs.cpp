#include "defs.h"
#include "init.h"
#include "combinations.h"

#include <set>
#include <iomanip>

unsigned max_value(Mask m) {
  assert(m.any() && "Unset mask");
  for (int i = static_cast<int>(m.size()) - 1; i >= 0; --i) {
    if (m[static_cast<unsigned>(i)])
      return static_cast<unsigned>(i + 1);
  }
  return 0;
}

unsigned min_value(Mask m) {
  assert(m.any() && "Unset mask");
  for (unsigned i = 0, e = m.size(); i != e; ++i) {
    if (m[static_cast<unsigned>(i)])
      return static_cast<unsigned>(i + 1);
  }
  return 0;
}

std::vector<Cage *> Cell::all_cages() {
  std::vector<Cage *> cages{cage};
  cages.insert(std::end(cages), std::begin(pseudo_cages),
               std::end(pseudo_cages));
  return cages;
}

std::vector<Cage const *> Cell::all_cages() const {
  std::vector<Cage const *> cages{cage};
  cages.insert(std::end(cages), std::begin(pseudo_cages),
               std::end(pseudo_cages));
  return cages;
}

House::~House() {}

bool House::contains(Cell const *cell) const {
  return cell->box == this || cell->row == this || cell->col == this;
}

std::string House::getPrintNum() const {
  return getRowID(num, USE_ROWCOL);
}

std::ostream &operator<<(std::ostream &os, House const &house) {
  if (USE_ROWCOL)
    os << house.getShortPrintKind() << house.getPrintNum();
  else
    os << house.getPrintKind() << " " << getRowID(house.num, USE_ROWCOL);
  return os;
}

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
      auto innie_outie = std::make_unique<InnieOutie>(cage->sum);

      // Collect cells found inside and outside the cage
      for (auto &cage_cell : *cage) {
        const Coord &coord = cage_cell->coord;
        if (coord.row >= min.row && coord.row <= max.row &&
            coord.col >= min.col && coord.col <= max.col) {
          innie_outie->inside_cage->cells.push_back(cage_cell);
        } else {
          innie_outie->outside_cage->cells.push_back(cage_cell);
        }
      }

      if (innie_outie->inside_cage->size() == cage->size()) {
        // Add to the known total if all cells are inside
        for (auto *innie_cell : *innie_outie->inside_cage)
          known_cage->cells.push_back(innie_cell);
        known_cage->sum += cage->sum;
        continue;
      }

      innies_outies.push_back(std::move(innie_outie));
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
  std::array<bool, 81> seen_cells;
  std::fill(seen_cells.begin(), seen_cells.end(), false);

  bool invalid = false;
  for (auto &cage : cages) {
    for (auto &cell : cage->cells) {
      const unsigned idx = cell->coord.row * 9 + cell->coord.col;
      if (seen_cells[idx]) {
        invalid = true;
        std::cerr << "Duplicated cell " << cell->coord << "\n";
      }
      seen_cells[idx] = true;
    }
  }

  for (unsigned row = 0; row < 9; ++row) {
    for (unsigned col = 0; col < 9; ++col) {
      auto *cell = getCell(row, col);
      if (cell->cage)
        continue;
      invalid = true;
      std::cerr << "No cage for " << cell->coord << "\n";
    }
  }

  return invalid;
}

bool Grid::validate() {
  bool invalid = false;
  unsigned total_sum = 0;
  for (auto &cage : cages) {
    if (cage->sum == 0) {
      invalid = true;
      std::cerr << "Error: cage sum is zero for cage containing "
                << cage->cells[0]->coord << "\n";
    }
    total_sum += cage->sum;
  }

  if (invalid)
    return true;

  if (total_sum != 405) {
    std::cerr << "Error: cage total (" << total_sum << ") is not 405\n";
    return true;
  }
  return false;
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

void Grid::writeToFile(std::ostream &file) {
  for (auto &row : rows) {
    bool sep = false;
    for (Cell *const cell : *row) {
      file << (sep ? " " : "") << "0x" << std::hex << std::setfill('0')
           << std::setw(3) << cell->candidates.to_ulong();
      sep = true;
    }
    file << "\n";
  }

  file << "\n";
  file << std::dec;

  for (auto &cage : cages) {
    file << std::setfill(' ') << std::setw(2) << std::left << cage->sum;
    for (auto *cell : cage->cells) {
      file << ' ' << getRowID(cell->coord.row, /*rowcol*/ false)
           << cell->coord.col;
    }
    file << "\n";
  }
  file.flush();
}

void Grid::initializeCageSubsetMap() {
  for (auto &cage : cages) {
    cage_combos.emplace_back(generateCageComboInfo(cage.get()));
    cage->cage_combos = cage_combos.back().get();
  }
}

void Grid::initializeInnieAndOutieRegions() {
  // Do column-oriented regions
  const int max_width = 4;
  for (unsigned width = 1; width <= max_width; ++width) {
    for (unsigned col = 0; col <= 9 - width; ++col) {
      auto region = std::make_unique<InnieOutieRegion>(
          Coord{0, col}, Coord{8, col + width - 1});
      region->initialize(this);
      if (region->known_cage->sum != region->expected_sum) {
        innies_and_outies.push_back(std::move(region));
        if (width == 1) {
          innies_and_outies.back()->house = cols[col].get();
          cols[col]->region = innies_and_outies.back().get();
        }
      }
    }
  }

  // Do row-oriented regions
  for (unsigned width = 1; width <= max_width; ++width) {
    for (unsigned row = 0; row <= 9 - width; ++row) {
      auto region = std::make_unique<InnieOutieRegion>(
          Coord{row, 0}, Coord{row + width - 1, 8});
      region->initialize(this);
      if (region->known_cage->sum != region->expected_sum) {
        innies_and_outies.push_back(std::move(region));
        if (width == 1) {
          innies_and_outies.back()->house = rows[row].get();
          rows[row]->region = innies_and_outies.back().get();
        }
      }
    }
  }

  for (unsigned ywidth = 1; ywidth <= 2; ++ywidth) {
    for (unsigned y = 0; y <= 3 - ywidth; ++y) {
      for (unsigned xwidth = 1; xwidth <= 2; ++xwidth) {
        for (unsigned x = 0; x <= 3 - xwidth; ++x) {
          auto region = std::make_unique<InnieOutieRegion>(
              Coord{y * 3, x * 3},
              Coord{y * 3 + (3 * ywidth) - 1, x * 3 + (3 * xwidth) - 1});
          region->initialize(this);
          if (region->known_cage->sum != region->expected_sum) {
            innies_and_outies.push_back(std::move(region));
            if (xwidth == 1 && ywidth == 1) {
              unsigned box_id = y * 3 + x;
              innies_and_outies.back()->house = boxes[box_id].get();
              boxes[box_id]->region = innies_and_outies.back().get();
            }
          }
        }
      }
    }
  }
}

std::ostream &operator<<(std::ostream &os, const Coord &coord) {
  os << (USE_ROWCOL ? "R" : "") << getRowID(coord.row, USE_ROWCOL)
     << (USE_ROWCOL ? "C" : "") << (coord.col + 1);
  return os;
}

Printable CellCageUnit::printCellList() const {
  if (cell)
    return Printable([c = cell](std::ostream &os) { os << c->coord; });
  else
    return cage->printCellList();
}

bool CellCageUnit::is_or_contains(Cell *c) const {
  return (cell && cell == c) || (cage && cage->contains(c));
}

bool CellCageUnit::overlapsWith(Cell const *c) const {
  return (cell && cell == c) || (cage && cage->contains(c));
}
bool CellCageUnit::overlapsWith(Cage const *c) const {
  return (cell && c->contains(cell)) ||
         (cage && cage->overlapsWith(c));
}
bool CellCageUnit::overlapsWith(CellCageUnit const &other) const {
  return (cell && other.cell && cell == other.cell) ||
         (cell && other.cage && other.cage->contains(cell)) ||
         (cage && other.cell && cage->contains(other.cell)) ||
         (cage && other.cage && cage->overlapsWith(other.cage));
}

std::ostream &operator<<(std::ostream &os, const CellCageUnit &unit) {
  if (unit.cell)
    os << unit.cell->coord;
  else
    os << *unit.cage;
  return os;
}

std::string InnieOutieRegion::getName() const {
  std::stringstream ss;
  if (house && house->getKind() == HouseKind::Col)
    ss << "C" << min.col + 1;
  else if (house && house->getKind() == HouseKind::Row)
    ss << "R" << min.row + 1;
  else if (house && house->getKind() == HouseKind::Box)
    ss << "B" << ((min.row / 3) * 3 + (min.col / 3)) + 1;
  else if (min.row == 0 && max.row == 8) {
    // Columns
    ss << "C";
    for (unsigned i = min.col; i <= max.col; i++)
      ss << i + 1;
  } else if (min.col == 0 && max.col == 8) {
    // Rows
    ss << "R";
    for (unsigned i = min.row; i <= max.row; i++)
      ss << i + 1;
  } else {
    // Boxes
    ss << "B";
    for (unsigned y = min.row; y < max.row; y += 3)
      for (unsigned x = min.col; x < max.col; x += 3)
        ss << ((y / 3) * 3 + (x / 3)) + 1;
  }
  return ss.str();
}

const char *invalid_grid_exception::what() const noexcept {
  return msg.c_str();
}
