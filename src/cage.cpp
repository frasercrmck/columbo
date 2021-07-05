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

static int doMinMaxHelper(std::vector<Cell const*> const &cells, unsigned idx, Tuple &tuple,
                          std::function<bool(int, int)> const &comparator,
                          int current_best,
                          std::vector<CellMask> const &clashes) {
  Cell const *cell = cells[idx];
  for (unsigned ci = 0, ce = cell->candidates.size(); ci != ce; ci++) {
    if (!cell->candidates[ci])
      continue;
    int val = ci + 1;
    if (hasClash(tuple, idx, val, clashes))
      continue;

    if (idx + 1 < cells.size()) {
      // Record this candidate and recurse to see if it generates a new
      // min/max.
      tuple.sum += val;
      tuple.values.push_back(val);
      int cand = doMinMaxHelper(cells, idx + 1, tuple, comparator, current_best,
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

int Cage::getMinMaxValue(bool is_min) const {
  // If we know for sure that the cage isn't a pseudo, the only value it can
  // hold it its own sum.
  // TODO: if sums were optional values, then even pseudos with "real" sums
  // could take a shortcut here.
  if (!is_pseudo)
    return sum;

  // Easy case.
  if (size() == 1)
    return (is_min ? min_value : max_value)(cells[0]->candidates);

  auto clashes = getCellClashMasks();

  // Try to split the cage up into multiple independent "groups" of cells which internally see
  // each other but where no cell sees across a group boundary. This is akin to
  // strongly-connected components. The min/max can be calculated on each group
  // independently and summed.
  unsigned next_group_idx = 0;
  unsigned no_group_val = std::numeric_limits<unsigned>::max();

  std::vector<unsigned> group_assigments(clashes.size(), no_group_val);

  std::vector<unsigned> worklist;
  std::unordered_set<unsigned> visited;
  for (unsigned i = 0, e = clashes.size(); i != e; i++) {
    // If we've already assigned a group, skip it
    if (group_assigments[i] != no_group_val)
      continue;

    // Construct a worklist to perform a depth-first search on cell reachability.
    worklist.push_back(i);
    visited.clear();
    visited.insert(i);

    while (!worklist.empty()) {
      auto idx = worklist.back();
      worklist.pop_back();
      for (unsigned j = 0; j != e; j++)
        if (idx != j && clashes[idx][j] && visited.insert(j).second)
          worklist.push_back(j);
    }

    // Everything in 'visited' is seeable from the current cell. Therefore they
    // must all be part of the same group.
    for (auto val : visited)
      group_assigments[val] = next_group_idx;
    // Bump the next group index
    next_group_idx++;
  }

  unsigned num_groups = next_group_idx;

  // We should always have at least one group: a blob containing the whole
  // cell.
  if (num_groups == 0)
    throw invalid_grid_exception{"No cell groups were found"};

  std::vector<std::vector<Cell const *>> groups(num_groups);

  // Construct the individual cell lists now.
  for (unsigned i = 0, e = size(); i != e; i++)
    groups[group_assigments[i]].push_back(cells[i]);

  int initial_val = is_min ? std::numeric_limits<int>::max()
                           : std::numeric_limits<int>::min();
  using ComparisonFnTy =std::function<bool(int, int)>;
  auto const &cmp_fn = is_min ? (ComparisonFnTy)std::less<int>()
                              : (ComparisonFnTy)std::greater<int>();

  int min_max = 0;
  for (auto const &group : groups) {
    Tuple tuple;
    tuple.values.reserve(group.size());
    min_max += doMinMaxHelper(group, 0, tuple, cmp_fn, initial_val, clashes);
  }

  return min_max;
}

int Cage::getMinValue() const {
  return getMinMaxValue(/*is_min*/true);
}

int Cage::getMaxValue() const {
  return getMinMaxValue(/*is_min*/false);
}
