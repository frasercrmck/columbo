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
};

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

using House = std::array<Cell *, 9>;
using HouseArray = std::array<House, 9>;

using Grid = std::array<std::array<Cell, 9>, 9>;

struct Cage {
  int8_t sum;
  std::vector<Cell *> cells;

  void addCells(Grid *const grid, std::initializer_list<Coord> coords);

  Cage(int8_t s, Grid *const grid, std::initializer_list<Coord> coords)
      : sum(s) {
    addCells(grid, coords);
  }

  std::vector<Cell *>::iterator end() { return cells.end(); }
  std::vector<Cell *>::iterator begin() { return cells.begin(); }
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
