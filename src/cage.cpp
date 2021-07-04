#include "defs.h"
#include "step.h"

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

// TODO: More efficient way of doing this?
std::optional<std::size_t> Cage::indexOf(Cell const *cell) const {
  for (unsigned i = 0, e = size(); i != e; i++)
    if (cells[i] == cell)
      return i;
  return std::nullopt;
}

std::ostream &operator<<(std::ostream &os, const Cage &cage) {
  os << cage.sum << "/" << cage.size();
  if (cage.is_pseudo && !cage.pseudo_name.empty())
    os << " (" << cage.pseudo_name << ")";
  return os;
}

Printable Cage::printCellList() const {
  return Printable([&](std::ostream &os) {
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
  });
}

Printable Cage::printMaskedCellList(CellMask const &mask) const {
  if (size() > mask.size())
    throw invalid_grid_exception{"Cage too large for mask"};
  return Printable([this, mask](std::ostream &os) {
    bool sep = false, list = false;
    for (unsigned i = 0, e = size(); i != e; i++) {
      if (mask[i]) {
        if (!list) {
          list = true;
          os << '(';
        }
        os << (sep ? "," : "") << cells[i]->coord;
        sep = true;
      }
    }
    if (list)
      os << ')';
  });
}

Printable Cage::printAnnotatedMaskedCellList(
    CellMask const &mask,
    std::unordered_map<unsigned, char> const &symbol_map) const {
  if (size() > mask.size())
    throw invalid_grid_exception{"Cage too large for mask"};
  return Printable([this, mask, symbol_map](std::ostream &os) {
    bool sep = false, list = false;
    for (unsigned i = 0, e = size(); i != e; i++) {
      if (mask[i]) {
        if (!list) {
          list = true;
          os << '(';
        }
        os << (sep ? "," : "") << cells[i]->coord;
        if (auto it = symbol_map.find(i); it != symbol_map.end())
          os << it->second;
        sep = true;
      }
    }
    if (list)
      os << ')';
  });
}

std::vector<CellMask> Cage::getCellClashMasks() const {
  std::vector<CellMask> clashes;
  for (auto const *cell : cells) {
    std::size_t i = 0;
    CellMask clash = 0;
    for (auto const *other_cell : cells) {
      if (cell != other_cell && cell->canSee(other_cell))
        clash.set(i);
      i++;
    }
    clashes.push_back(clash);
  }
  return clashes;
}

struct Tuple {
  int sum = 0;
  IntList values;

  std::size_t size() const { return values.size(); }

  decltype(values)::iterator end() { return values.end(); }
  decltype(values)::iterator begin() { return values.begin(); }

  decltype(values)::const_iterator end() const { return values.end(); }
  decltype(values)::const_iterator begin() const { return values.begin(); }
};

static bool hasClash(Tuple const &tuple, int tuple_index, int candidate,
                     std::vector<CellMask> const &clashes) {
  for (unsigned i = 0, e = tuple.size(); i != e; i++)
    if (tuple.values[i] == candidate && clashes[tuple_index][i])
      return true;
  return false;
}

static int doMinMaxHelper(Cage const &cage, unsigned idx, Tuple &tuple,
                          std::function<bool(int, int)> const &comparator,
                          int current_best,
                          std::vector<CellMask> const &clashes) {
  Cell const *cell = cage[idx];
  for (unsigned ci = 0, ce = cell->candidates.size(); ci != ce; ci++) {
    if (!cell->candidates[ci])
      continue;
    int val = ci + 1;
    if (hasClash(tuple, idx, val, clashes))
      continue;

    if (idx + 1 < cage.size()) {
      // Record this candidate and recurse to see if it generates a new
      // min/max.
      tuple.sum += val;
      tuple.values.push_back(val);
      int cand = doMinMaxHelper(cage, idx + 1, tuple, comparator, current_best,
                                clashes);
      if (comparator(cand, current_best))
        current_best = cand;
      // Remember to pop it off again...
      tuple.sum -= val;
      tuple.values.pop_back();
    } else if (comparator(tuple.sum + val, current_best)) {
      current_best = tuple.sum + val;
    }
  }
  return current_best;
}

int Cage::getMinValue() const {
  // If we know for sure that the cage isn't a pseudo, the only value it can
  // hold it its own sum.
  // TODO: if sums were optional values, then even pseudos with "real" sums
  // could take a shortcut here.
  if (!is_pseudo)
    return sum;

  // Easy case.
  if (size() == 1)
    return min_value(cells[0]->candidates);

  auto clashes = getCellClashMasks();

  Tuple tuple;
  tuple.values.reserve(size());
  return doMinMaxHelper(*this, 0, tuple, std::less<int>(),
                        std::numeric_limits<int>::max(), clashes);
}

int Cage::getMaxValue() const {
  if (!is_pseudo)
    return sum;

  if (size() == 1)
    return max_value(cells[0]->candidates);

  auto clashes = getCellClashMasks();

  Tuple tuple;
  tuple.values.reserve(size());
  return doMinMaxHelper(*this, 0, tuple, std::greater<int>(),
                        std::numeric_limits<int>::min(), clashes);
}
