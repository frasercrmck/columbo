#ifndef SOLVER_DEFS_H
#define SOLVER_DEFS_H

#include <bitset>
#include <vector>
#include <array>

using CandidateSet = std::bitset<9>;

using IntList = std::vector<int>;

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
  HouseKind kind;
  std::array<Cell *, 9> cells;

  House(HouseKind k) : kind(k) {}

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
  Row() : House(HouseKind::Row) {}
  unsigned getLinearID(const Cell *const cell) const override;
};

struct Col : House {
  Col() : House(HouseKind::Col) {}
  unsigned getLinearID(const Cell *const cell) const override;
};

struct Box : House {
  Box() : House(HouseKind::Box) {}
  unsigned getLinearID(const Cell *const cell) const override;
};

using HouseArray = std::array<std::unique_ptr<House>, 9>;

using Grid = std::array<std::array<Cell, 9>, 9>;

struct Cage {
  int sum = 0;
  int colour = 0;
  std::vector<Cell *> cells;

  void addCells(Grid *const grid, std::initializer_list<Coord> coords);

  Cage() : sum(0) {}

  Cage(int s, Grid *const grid, std::initializer_list<Coord> coords) : sum(s) {
    addCells(grid, coords);
  }

  std::vector<Cell *>::iterator end() { return cells.end(); }
  std::vector<Cell *>::iterator begin() { return cells.begin(); }

  bool empty() const { return cells.empty(); }
  std::size_t size() const { return cells.size(); }
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

#endif // SOLVER_DEFS_H
