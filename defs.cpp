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
  return (kind == HouseKind::Row && cell->coord.row == num) ||
         (kind == HouseKind::Col && cell->coord.col == num) ||
         (kind == HouseKind::Box && cell->coord.row / 3 == num / 3 &&
          cell->coord.col / 3 == num % 3);
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
        std::cerr << "Duplicated cell " << cell->coord << "\n";
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
      std::cerr << "No cage for " << cell->coord << "\n";
    }
  }

  return invalid;
}

bool Grid::validate() {
  unsigned total_sum = 0;
  for (auto &cage : cages)
    total_sum += cage->sum;

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
    for (unsigned i = 0, e = row->size(); i != e; i++) {
      file << "0x" << std::hex << std::setfill('0') << std::setw(3)
           << (*row)[i]->candidates.to_ulong();
      if (i < e - 1)
        file << ' ';
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
    std::vector<Mask> possibles;
    possibles.reserve(cage->cells.size());
    for (auto const *cell : cage->cells)
      possibles.push_back(cell->candidates);

    auto subsets = generateCageSubsetSums(cage->sum, possibles);

    // As a stop-gap, expand permutations here.
    for (CageCombo &cage_combo : subsets)
      expandComboPermutations(cage.get(), cage_combo);

    cage_combos.emplace_back(
        std::make_unique<CageComboInfo>(cage.get(), std::move(subsets)));

    cage->cage_combos = cage_combos.back().get();
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
      if (region->known_cage.sum != region->expected_sum) {
        innies_and_outies.push_back(std::move(region));
        if (width == 1) {
          innies_and_outies.back()->house = rows[row].get();
          rows[row]->region = innies_and_outies.back().get();
        }
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
        unsigned box_id = y * 3 + x;
        innies_and_outies.back()->house = boxes[box_id].get();
        boxes[box_id]->region = innies_and_outies.back().get();
      }
    }
  }
}

std::ostream &operator<<(std::ostream &os, const Coord &coord) {
  os << (USE_ROWCOL ? "R" : "") << getRowID(coord.row, USE_ROWCOL)
     << (USE_ROWCOL ? "C" : "") << (coord.col + 1);
  return os;
}

std::ostream &operator<<(std::ostream &os, const Cage &cage) {
  os << cage.sum << "/" << cage.size();
  if (cage.is_pseudo && !cage.pseudo_name.empty())
    os << " (" << cage.pseudo_name << ")";
  return os;
}

void Cage::printCellList(std::ostream &os) const {
  if (size() == 1) {
    os << cells[0]->coord;
  } else {
    os << '(';
    for (unsigned i = 0, e = size(); i != e; i++) {
      os << cells[i]->coord;
      if (i < e - 1)
        os << ',';
    }
    os << ')';
  }
}

void CellCageUnit::printCellList(std::ostream &os) const {
  if (cell)
    os << cell->coord;
  else
    cage->printCellList(os);
}

bool CellCageUnit::is_or_contains(Cell *c) const {
  return (cell && cell == c) || (cage && cage->member_set().count(c));
}

bool CellCageUnit::overlapsWith(Cell *c) const {
  return (cell && cell == c) || (cage && cage->member_set().count(c));
}
bool CellCageUnit::overlapsWith(Cage *c) const {
  return (cell && c->member_set().count(cell)) ||
         (cage && std::any_of(std::begin(*cage), std::end(*cage),
                              [the_cage = c](Cell *cx) {
                                return the_cage->member_set().count(cx);
                              }));
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
  else
    ss << "[" << min << " - " << max << "]";
  return ss.str();
}

std::unordered_set<Cell *> Cage::member_set() {
  return std::unordered_set<Cell *>{std::begin(cells), std::end(cells)};
}

std::unordered_set<Cell const *> Cage::member_set() const {
  return std::unordered_set<Cell const *>{std::begin(cells), std::end(cells)};
}

bool Cage::doAllCellsSeeEachOther() const {
  for (std::size_t c1 = 0, ce = size(); c1 < ce; ++c1)
    for (std::size_t c2 = c1 + 1; c2 < ce; ++c2)
      if (!cells[c1]->canSee(cells[c2]))
        return false;
  return true;
}

bool Cage::areAllCellsAlignedWith(House const &house) const {
  return std::all_of(
      std::begin(cells), std::end(cells),
      [house](Cell const *cell) { return house.contains(cell); });
}
