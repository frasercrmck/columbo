#include "defs.h"

void Cage::addCell(Cell *cell) {
  cells.push_back(cell);
  if (!is_pseudo)
    cell->cage = this;
  else if (std::find(std::begin(cell->pseudo_cages),
                     std::end(cell->pseudo_cages),
                     this) == std::end(cell->pseudo_cages))
    cell->pseudo_cages.push_back(this);
}

void Cage::addCell(Grid *const grid, Coord coord) {
  addCell(grid->getCell(coord));
}

bool Cage::contains(Cell *c) const {
  return std::find(std::begin(cells), std::end(cells), c) != std::end(cells);
}
bool Cage::contains(Cell const *c) const {
  return std::find(std::begin(cells), std::end(cells), c) != std::end(cells);
}

std::unordered_set<Cell *> Cage::member_set() {
  return std::unordered_set<Cell *>{std::begin(cells), std::end(cells)};
}

std::unordered_set<Cell const *> Cage::member_set() const {
  return std::unordered_set<Cell const *>{std::begin(cells), std::end(cells)};
}

bool Cage::overlapsWith(Cage const *c) const {
  return std::any_of(std::begin(cells), std::end(cells),
                     [c](Cell *cx) { return c->contains(cx); });
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
      [&house](Cell const *cell) { return house.contains(cell); });
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
    bool sep = false;
    for (auto const *cell : *this) {
      os << (sep ? "," : "") << cell->coord;
      sep = true;
    }
    os << ')';
  }
}
