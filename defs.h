#ifndef COLUMBO_DEFS_H
#define COLUMBO_DEFS_H

#include <bitset>
#include <vector>
#include <array>

using Mask = unsigned long;

using CandidateSet = std::bitset<9>;

using IntList = std::vector<unsigned>;

struct Coord {
  unsigned row;
  unsigned col;
  bool operator==(const Coord &other) const {
    return row == other.row && col == other.col;
  }
};

std::ostream &operator<<(std::ostream &os, const Coord &coord);

struct Cage;

using CageList = std::vector<Cage>;

struct Cell {
  Cage *cage;
  Coord coord;
  CandidateSet candidates;
  Cell() : cage(nullptr), candidates() {
    /* Set all candidates by default */
    candidates.set();
  }

  unsigned isFixed() const {
    if (candidates.count() != 1) {
      return 0;
    }
    for (unsigned i = 0; i < 9; ++i) {
      if (candidates.test(i)) {
        return i + 1;
      }
    }
    return 0;
  }
};

enum class HouseKind { Row, Col, Box };

struct House {
  unsigned num;
  HouseKind kind;
  std::array<Cell *, 9> cells;

  House(unsigned n, HouseKind k) : num(n), kind(k) {}

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

  HouseKind getKind() const { return kind; }

  std::size_t size() const { return 9; }

  Cell *&operator[](std::size_t i) { return cells[i]; }

  std::array<Cell *, 9>::iterator end() { return cells.end(); }
  std::array<Cell *, 9>::iterator begin() { return cells.begin(); }

  std::array<Cell *, 9>::const_iterator end() const { return cells.end(); }
  std::array<Cell *, 9>::const_iterator begin() const { return cells.begin(); }
};

struct Row : House {
  Row(unsigned num) : House(num, HouseKind::Row) {}
  unsigned getLinearID(const Cell *const cell) const override;
};

struct Col : House {
  Col(unsigned num) : House(num, HouseKind::Col) {}
  unsigned getLinearID(const Cell *const cell) const override;
};

struct Box : House {
  Box(unsigned num) : House(num, HouseKind::Box) {}
  unsigned getLinearID(const Cell *const cell) const override;
};

using HouseArray = std::array<std::unique_ptr<House>, 9>;

struct InnieOutieRegion;

struct Grid {
  std::array<std::array<Cell, 9>, 9> cells;

  HouseArray rows;
  HouseArray cols;
  HouseArray boxes;

  CageList cages;

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
            (*boxes[a])[b] = &cells[c][d];
          }
        }
      }
    }
  }

  Cell *getCell(const Coord &coord);

  Cell *getCell(unsigned y, unsigned x);

  void initialize();

private:
  void initializeInnieAndOutieRegions();
};

struct Cage {
  unsigned sum = 0;
  int colour = 0;
  std::vector<Cell *> cells;

  void addCells(Grid *const grid, std::initializer_list<Coord> coords);

  Cage() : sum(0) {}

  Cage(unsigned s, Grid *const grid, std::initializer_list<Coord> coords) : sum(s) {
    addCells(grid, coords);
  }

  std::vector<Cell *>::iterator end() { return cells.end(); }
  std::vector<Cell *>::iterator begin() { return cells.begin(); }

  bool empty() const { return cells.empty(); }
  std::size_t size() const { return cells.size(); }
};

struct InnieOutieRegion {
  Coord min;
  Coord max;

  Cage innie_cage;
  Cage outie_cage;
  Cage known_cage;
  Cage unknown_cage;

  unsigned num_cells;
  unsigned expected_sum;

  InnieOutieRegion(Coord _min, Coord _max) : min(_min), max(_max) {
    unsigned rows = 1 + max.row - min.row;
    unsigned cols = 1 + max.col - min.col;

    num_cells = rows * cols;
    expected_sum = rows * cols * 5;
  }

  void initialize(Grid *const grid);
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
