#ifndef COLUMBO_DEFS_H
#define COLUMBO_DEFS_H

#include <array>
#include <bitset>
#include <map>
#include <memory>
#include <algorithm>
#include <set>
#include <sstream>
#include <unordered_set>
#include <vector>

using Mask = std::bitset<9>;

unsigned max_value(Mask m);
unsigned min_value(Mask m) ;

using CandidateSet = std::bitset<9>;

// A mask that has one index dedicated to each cell in a given cage. If any
// cage is larger than 32 cells, this can change.
using CellMask = std::bitset<32>;

using IntList = std::vector<unsigned>;

struct CageCombo {
  explicit CageCombo(Mask m) : combo(m) {}
  Mask combo;
  Mask duplicates = 0;
  std::vector<IntList> permutations = {};
};

struct Cell;
struct Cage;
struct House;

struct CageComboInfo {
  CageComboInfo(Cage const *cage, std::vector<CageCombo> &&combos)
      : cage(cage), combos(combos) {}

  std::size_t size() const { return combos.size(); }

  std::vector<CageCombo>::iterator end() { return combos.end(); }
  std::vector<CageCombo>::iterator begin() { return combos.begin(); }

  std::vector<CageCombo>::const_iterator end() const { return combos.end(); }
  std::vector<CageCombo>::const_iterator begin() const {
    return combos.begin();
  }

  std::unordered_set<Mask> computeKillerPairs(unsigned max_size) const;
  std::unordered_set<Mask>
  computeKillerPairs(unsigned max_size, CellMask const &cell_mask) const;
  std::unordered_set<Mask> getUniqueCombinations() const;
  std::unordered_set<Mask> getUniqueCombinationsIn(House const &house) const;
  std::unordered_set<Mask>
  getUniqueCombinationsWhichSee(Cell const *cell) const;

  Cage const *cage;
  std::vector<CageCombo> combos;
private:
  std::unordered_set<Mask>
  getUniqueCombinationsWithMask(CellMask const &cage_cell_mask) const;
};

struct Coord {
  unsigned row;
  unsigned col;
  bool operator==(const Coord &other) const {
    return row == other.row && col == other.col;
  }
};

std::ostream &operator<<(std::ostream &os, const Coord &coord);

using CageList = std::vector<std::unique_ptr<Cage>>;

struct Cell {
  Cage *cage;
  Coord coord;
  CandidateSet candidates;
  std::vector<Cage *> pseudo_cages;
  Cell() : cage(nullptr), candidates() {
    /* Set all candidates by default */
    candidates.set();
  }

  std::vector<Cage *> all_cages();
  std::vector<Cage const *> all_cages() const;

  const House *row = nullptr;
  const House *col = nullptr;
  const House *box = nullptr;

  bool canSee(const Cell *other) const {
    return (cage == other->cage || row == other->row || col == other->col ||
            box == other->box);
  }

  unsigned isFixed() const {
    if (candidates.count() != 1) {
      return 0;
    }
    for (unsigned i = 0; i < 9; ++i) {
      if (candidates[i]) {
        return i + 1;
      }
    }
    return 0;
  }
};

using CellSet = std::set<Cell *>;

struct InnieOutieRegion;

enum class HouseKind { Row, Col, Box };

struct House {
  unsigned num;
  HouseKind kind;
  std::array<Cell *, 9> cells;

  House(unsigned n, HouseKind k) : num(n), kind(k) {}

  House(House &&) = delete;
  House(House const &) = delete;

  virtual ~House();

  virtual unsigned getLinearID(const Cell *const) const { return 0; }

  const char *getPrintKind() const {
    switch (kind) {
    case HouseKind::Row:
      return "row";
    case HouseKind::Col:
      return "column";
    case HouseKind::Box:
      return "box";
    }
  }

  std::string getPrintNum() const;

  const char *getShortPrintKind() const {
    switch (kind) {
    case HouseKind::Row:
      return "R";
    case HouseKind::Col:
      return "C";
    case HouseKind::Box:
      return "B";
    }
  }

  HouseKind getKind() const { return kind; }

  std::size_t size() const { return 9; }

  Cell *&operator[](std::size_t i) { return cells[i]; }

  std::array<Cell *, 9>::iterator end() { return cells.end(); }
  std::array<Cell *, 9>::iterator begin() { return cells.begin(); }

  std::array<Cell *, 9>::const_iterator end() const { return cells.end(); }
  std::array<Cell *, 9>::const_iterator begin() const { return cells.begin(); }

  bool contains(Cell const *cell) const;

  InnieOutieRegion *region = nullptr;
};

std::ostream &operator<<(std::ostream &os, House const &house);

struct Row : House {
  Row(unsigned n) : House(n, HouseKind::Row) {}
  unsigned getLinearID(const Cell *const cell) const override;
};

struct Col : House {
  Col(unsigned n) : House(n, HouseKind::Col) {}
  unsigned getLinearID(const Cell *const cell) const override;
};

struct Box : House {
  Box(unsigned n) : House(n, HouseKind::Box) {}
  unsigned getLinearID(const Cell *const cell) const override;
};

using HouseArray = std::array<std::unique_ptr<House>, 9>;

struct Grid {
  std::array<std::array<Cell, 9>, 9> cells;

  HouseArray rows;
  HouseArray cols;
  HouseArray boxes;

  CageList cages;
  CageList pseudo_cages;

  std::vector<std::unique_ptr<CageComboInfo>> cage_combos;
  std::vector<std::unique_ptr<InnieOutieRegion>> innies_and_outies;

  Grid() {
    for (unsigned i = 0; i < 9; ++i) {
      rows[i] = std::make_unique<Row>(i);
      cols[i] = std::make_unique<Col>(i);
      boxes[i] = std::make_unique<Box>(i);
    }

    for (unsigned row = 0; row < 9; ++row) {
      for (unsigned col = 0; col < 9; ++col) {
        cells[row][col].coord = {row, col};
        cells[row][col].row = &*rows[row];
        cells[row][col].col = &*cols[col];
        (*rows[row])[col] = &cells[row][col];
        (*cols[row])[col] = &cells[col][row];
      }
    }

    for (unsigned y = 0; y < 3; ++y) {
      for (unsigned x = 0; x < 3; ++x) {
        for (unsigned z = 0; z < 3; ++z) {
          for (unsigned w = 0; w < 3; ++w) {
            unsigned a = y * 3 + x;
            unsigned b = z * 3 + w;
            unsigned c = y * 3 + z;
            unsigned d = x * 3 + w;
            cells[c][d].box = &*boxes[a];
            (*boxes[a])[b] = &cells[c][d];
          }
        }
      }
    }
  }

  Cell *getCell(const Coord &coord);

  Cell *getCell(unsigned y, unsigned x);

  bool initialize(std::ifstream &file, bool v = true);

  void writeToFile(std::ostream &file);

  void assignCageColours();

private:

  bool validate();
  bool initializeCages();
  void initializeCageSubsetMap();
  void initializeInnieAndOutieRegions();
};

struct Cage {
  unsigned sum = 0;
  int colour = 0;
  bool is_pseudo = false;
  std::vector<Cell *> cells;
  CageComboInfo *cage_combos = nullptr;
  std::string pseudo_name = "";

  void addCell(Cell *cell);
  void addCell(Grid *const grid, Coord coord);

  Cage() : sum(0) {}

  Cage(unsigned s) : sum(s) {}

  Cage(Cage &&other) = delete;
  Cage(Cage const &other) = delete;

  ~Cage() {
    for (auto *c : cells) {
      if (c->cage == this)
        c->cage = nullptr;

      c->pseudo_cages.erase(
          std::remove_if(std::begin(c->pseudo_cages), std::end(c->pseudo_cages),
                         [this](Cage const *cage) { return cage == this; }),
          std::end(c->pseudo_cages));
    }
  }

  std::vector<Cell *>::iterator end() { return cells.end(); }
  std::vector<Cell *>::iterator begin() { return cells.begin(); }

  std::vector<Cell *>::const_iterator end() const { return cells.end(); }
  std::vector<Cell *>::const_iterator begin() const { return cells.begin(); }

  bool contains(Cell *c) const;
  bool contains(Cell const *c) const;

  std::unordered_set<Cell *> member_set();
  std::unordered_set<Cell const *> member_set() const;

  Cell *operator[](std::size_t idx) { return cells[idx]; }
  Cell const *operator[](std::size_t idx) const { return cells[idx]; }

  bool empty() const { return cells.empty(); }
  std::size_t size() const { return cells.size(); }

  bool overlapsWith(Cage const *cage) const;

  bool doAllCellsSeeEachOther() const;

  bool areAllCellsAlignedWith(House const &) const;

  void printCellList(std::ostream &os) const;

  std::optional<std::size_t> indexOf(Cell const *cell) const;
};

std::ostream &operator<<(std::ostream &os, const Cage &cage);

struct CellCageUnit {
  CellCageUnit() = default;
  CellCageUnit(Cell *cell) : cell(cell), cage(nullptr) {}
  CellCageUnit(Cage *cage) : cell(nullptr), cage(cage) {}
  std::vector<Cell *> getCells() const {
    if (cell)
      return {cell};
    return cage->cells;
  }

  bool is_or_contains(Cell *c) const;
  bool overlapsWith(Cell const *c) const;
  bool overlapsWith(Cage const *cage) const;
  bool overlapsWith(CellCageUnit const &unit) const;

  bool operator==(CellCageUnit &other) const {
    return cell == other.cell && cage == other.cage;
  }
  bool operator==(CellCageUnit const &other) const {
    return cell == other.cell && cage == other.cage;
  }

  bool operator==(Cell *c) const { return cell == c && !cage; }
  bool operator==(Cell const *c) const { return cell == c && !cage; }

  bool operator==(Cage *c) const { return !cell && c == cage; }
  bool operator==(Cage const *c) const { return !cell && c == cage; }

  const char *getName() const { return cell ? "cell" : "cage"; }

  void printCellList(std::ostream &os) const;

  Cell *cell = nullptr;
  Cage *cage = nullptr;
};

std::ostream &operator<<(std::ostream &os, const CellCageUnit &unit);

struct InnieOutie {
  InnieOutie() = delete;
  InnieOutie(unsigned s)
      : sum(s), inside_cage(std::make_unique<Cage>()),
        outside_cage(std::make_unique<Cage>()) {}
  unsigned sum = 0;     // The sum of the original cage
  // The "interesting" cells; innies or outies
  std::unique_ptr<Cage> inside_cage = std::make_unique<Cage>();
  // The sibling cells from the same cage as the cell_cage
  std::unique_ptr<Cage> outside_cage = std::make_unique<Cage>();
};

struct InnieOutieRegion {
  Coord min;
  Coord max;

  // Cells whose contributions to the sum are known
  std::unique_ptr<Cage> known_cage = std::make_unique<Cage>();
  std::vector<std::unique_ptr<InnieOutie>> innies_outies;

  // If the region matches 1:1 with a row/col/box, this is it.
  House *house = nullptr;

  unsigned num_cells;
  unsigned expected_sum;

  InnieOutieRegion(Coord _min, Coord _max) : min(_min), max(_max) {
    unsigned rows = 1 + max.row - min.row;
    unsigned cols = 1 + max.col - min.col;

    num_cells = rows * cols;
    expected_sum = rows * cols * 5;
  }

  void initialize(Grid *const grid);

  std::string getName() const;

  std::vector<std::unique_ptr<Cage>> innies;
  std::vector<std::unique_ptr<Cage>> large_innies;
  std::vector<std::unique_ptr<Cage>> large_outies;
};

enum id {
  A = 0,
  B = 1,
  C = 2,
  D = 3,
  E = 4,
  F = 5,
  G = 6,
  H = 7,
  /* I looks like 1 */
  J = 8
};

#endif // COLUMBO_DEFS_H
