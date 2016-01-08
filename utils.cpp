#include "utils.h"

#include <iostream>

// Given a list of lists of possible cage values:
//     [[1,2,3], [3,4,5]]
// Recursively generates tuples of combinations from each of the lists as
// follows:
//   [1,3]
//   [1,4]
//   [1,5]
//   [2,3]
//   [2,4]
// ... etc
// Each of these is checked against the target sum, and pushed into a result
// vector if they match.
static void subsetSum(const std::vector<IntList> &possible_lists,
                      IntList &tuple, std::vector<IntList> &subsets,
                      const int target_sum, unsigned list_idx) {
  // Only try checking we've reach a sum when we've collected enough
  if (tuple.size() == possible_lists.size()) {
    int sum = 0;
    for (auto &x : tuple) {
      sum += x;
    }
    if (sum == target_sum) {
      subsets.push_back(tuple);
    }
  }

  for (unsigned p = list_idx; p < possible_lists.size(); ++p) {
    auto &possible_list = possible_lists[p];
    for (auto &poss : possible_list) {
      // Can't repeat a value inside a cage
      if (std::find(tuple.begin(), tuple.end(), poss) != tuple.end()) {
        continue;
      }
      tuple.push_back(poss);
      subsetSum(possible_lists, tuple, subsets, target_sum, p + 1);
      tuple.pop_back();
    }
  }
}

void generateSubsetSums(const int target_sum,
                        const std::vector<IntList> &possibles,
                        std::vector<IntList> &subsets) {
  IntList tuple;
  subsetSum(possibles, tuple, subsets, target_sum, 0);
}

Cell *getCell(Grid *const grid, unsigned y, unsigned x) {
  return &(*grid)[y][x];
}

Cell *getCell(Grid *const grid, const Coord &coord) {
  return getCell(grid, coord.row, coord.col);
}

int verify(Grid *grid, CageList &cages) {
  // Verify board
  int total = 0;
  for (auto &cage : cages) {
    total += cage.sum;
  }

  unsigned num_unused = 0;
  for (auto &row : *grid) {
    for (auto &cell : row) {
      if (!cell.cage) {
        ++num_unused;
        std::cout << "Not used: {" << cell.coord.row << "," << cell.coord.col
                  << "}\n";
      }
    }
  }

  if (num_unused) {
    return true;
  }

  if (total != 405) {
    std::cout << "Error: Total (" << total << ") is not 405\n";
    return true;
  }

  return false;
}

/*
static const char *LIGHT_DOUBLE_DASH_HORIZONTAL = "\u254c"; // '╌'
*/
static const char *LIGHT_DOUBLE_DASH_VERTICAL = "\u254e"; // '╎'
static const char *DOUBLE_HORIZONTAL = "\u2550";          // '═'
static const char *DOUBLE_VERTICAL = "\u2551";            // '║'
static const char *DOUBLE_DOWN_AND_RIGHT = "\u2554";      // '╔'
static const char *DOUBLE_DOWN_AND_LEFT = "\u2557";       // '╗'
static const char *DOUBLE_UP_AND_RIGHT = "\u255a";        // '╚'
static const char *DOUBLE_UP_AND_LEFT = "\u255d";         // '╝'

static const char *DOUBLE_VERTICAL_AND_RIGHT = "\u2560"; // '╠'
static const char *DOUBLE_VERTICAL_AND_LEFT = "\u2563";  // '╣'

static const char *VERTICAL_DOUBLE_AND_RIGHT_SINGLE = "\u255f"; // '╟'
static const char *VERTICAL_DOUBLE_AND_LEFT_SINGLE = "\u2562";  // '╢'

static const char *DOUBLE_DOWN_AND_HORIZONTAL = "\u2566"; // '╦'
static const char *DOUBLE_UP_AND_HORIZONTAL = "\u2569";   // '╩'

static const char *DOUBLE_VERTICAL_AND_HORIZONTAL = "\u256c";        // '╬'
static const char *VERTICAL_SINGLE_AND_HORIZONTAL_DOUBLE = "\u256a"; // '╤'
static const char *VERTICAL_DOUBLE_AND_HORIZONTAL_SINGLE = "\u256b"; // '╫'

static const char *DOWN_SINGLE_AND_HORIZONTAL_DOUBLE = "\u2564"; // '╤'
static const char *UP_SINGLE_AND_HORIZONTAL_DOUBLE = "\u2567";   // '╧'

static const char *LIGHT_VERTICAL_AND_HORIZONTAL = "\u253c"; // '┼'

/*
static const char *LIGHT_UP = "\u2575";         // '╵'
static const char *LIGHT_LEFT = "\u2574";       // '╴'
*/
static const char *LIGHT_VERTICAL = "\u2502";   // '│'
static const char *LIGHT_HORIZONTAL = "\u2500"; // '─'

void printLine(const Grid *const grid, const unsigned row, const bool big_grid,
               const bool thick, const bool top, const bool bottom,
               const bool use_colour) {
  // Indent three spaces
  std::cout << "   ";

  // Print the initial box boundary
  const char *beg_vert_glyph = VERTICAL_DOUBLE_AND_RIGHT_SINGLE;
  if (thick) {
    if (top) {
      beg_vert_glyph = DOUBLE_DOWN_AND_RIGHT;
    } else if (bottom) {
      beg_vert_glyph = DOUBLE_UP_AND_RIGHT;
    } else {
      beg_vert_glyph = DOUBLE_VERTICAL_AND_RIGHT;
    }
  }
  std::cout << beg_vert_glyph;

  for (unsigned x = 0; x < 9; ++x) {
    // Print three horizontal lines
    const Cell &this_cell = (*grid)[row][x];
    const bool down_is_cage_buddy =
        !top && row != 8 && this_cell.cage == (*grid)[row + 1][x].cage;
    const char *horiz_glyph = LIGHT_HORIZONTAL;
    if (down_is_cage_buddy && use_colour) {
      std::cout << "\x1b[4" << this_cell.cage->colour << "m";
    }
    if (thick) {
      horiz_glyph = DOUBLE_HORIZONTAL;
    }
    // Small grid prints each cell 2 characters wide
    for (int z = 0; z < (big_grid ? 3 : 2); ++z) {
      std::cout << horiz_glyph;
    }

    if (down_is_cage_buddy && use_colour) {
      // Reset attributes
      std::cout << "\x1b[0m";
    }

    if (x == 2 || x == 5) {
      // If we're at a box intersection...
      const char *intersect_glyph = VERTICAL_DOUBLE_AND_HORIZONTAL_SINGLE;
      if (thick) {
        if (top) {
          intersect_glyph = DOUBLE_DOWN_AND_HORIZONTAL;
        } else if (bottom) {
          intersect_glyph = DOUBLE_UP_AND_HORIZONTAL;
        } else {
          intersect_glyph = DOUBLE_VERTICAL_AND_HORIZONTAL;
        }
      }
      std::cout << intersect_glyph;
    } else if (x != 8) {
      // If we're at a regular cell intersection...
      const char *intersect_glyph = LIGHT_VERTICAL_AND_HORIZONTAL;
      if (thick) {
        if (top) {
          intersect_glyph = DOWN_SINGLE_AND_HORIZONTAL_DOUBLE;
        } else if (bottom) {
          intersect_glyph = UP_SINGLE_AND_HORIZONTAL_DOUBLE;
        } else {
          intersect_glyph = VERTICAL_SINGLE_AND_HORIZONTAL_DOUBLE;
        }
      }
      std::cout << intersect_glyph;
    }
  }

  // Print the end of the line: a vertical bar
  const char *end_vert_glyph = VERTICAL_DOUBLE_AND_LEFT_SINGLE;

  if (thick) {
    if (top) {
      end_vert_glyph = DOUBLE_DOWN_AND_LEFT;
    } else if (bottom) {
      end_vert_glyph = DOUBLE_UP_AND_LEFT;
    } else {
      end_vert_glyph = DOUBLE_VERTICAL_AND_LEFT;
    }
  }

  std::cout << end_vert_glyph;
}

void printBigGridCell(const Cell &cell, int row, bool use_colour) {
  if (use_colour) {
    // Coloured background
    std::cout << "\x1b[4" << cell.cage->colour << "m";
    // Black foreground
    std::cout << "\x1b[30m";
  }

  if (cell.isFixed()) {
    if (row != 1) {
      std::cout << "   ";
    } else {
      std::cout << " " << cell.isFixed() << " ";
    }
  } else {
    for (int i = row * 3, e = i + 3; i < e; ++i) {
      if (cell.candidates[static_cast<std::size_t>(i)]) {
        std::cout << i + 1;
      } else {
        std::cout << " ";
      }
    }
  }

  if (use_colour) {
    // Reset attributes
    std::cout << "\x1b[0m";
  }
}

static void printSmallGridCell(const Cell &cell, bool use_colour) {
  if (use_colour) {
    // Coloured background
    std::cout << "\x1b[4" << cell.cage->colour << "m";
    // Black foreground
    std::cout << "\x1b[30m";
  }

  Coord top_left = {9, 9};
  for (auto &other : *cell.cage) {
    if (other->coord.row < top_left.row) {
      top_left = other->coord;
    }
  }

  if (cell.coord == top_left) {
    if (cell.cage->sum < 10) {
      std::cout << " ";
    }
    std::cout << cell.cage->sum;
  } else {
    std::cout << "  ";
  }

  if (use_colour) {
    // Reset attributes
    std::cout << "\x1b[0m";
  }
}

void printRow(const std::array<Cell, 9> &house, unsigned row, int sub_row,
              bool big_grid, bool use_colour) {
  std::cout << " ";
  if (sub_row == 1) {
    std::cout << getID(row);
  } else {
    std::cout << " ";
  }
  std::cout << " ";
  std::cout << DOUBLE_VERTICAL;
  for (unsigned col = 0; col < 9; ++col) {
    if (big_grid) {
      printBigGridCell(house[col], sub_row, use_colour);
    } else {
      printSmallGridCell(house[col], use_colour);
    }
    const Cell &this_cell = house[col];
    const char *vert_glyph = LIGHT_DOUBLE_DASH_VERTICAL;
    const bool right_is_cage_buddy =
        col != 8 && this_cell.cage == house[col + 1].cage;
    const bool is_thick = col == 2 || col == 5 || col == 8;
    if (right_is_cage_buddy && use_colour) {
      std::cout << "\x1b[4" << this_cell.cage->colour << "m";
    }
    if (is_thick) {
      vert_glyph = DOUBLE_VERTICAL;
    } else {
      vert_glyph = LIGHT_VERTICAL;
    }
    std::cout << vert_glyph;

    if (right_is_cage_buddy && use_colour) {
      // Reset attributes
      std::cout << "\x1b[0m";
    }
  }
}

void printGrid(const Grid *const grid, bool use_colour, const char *phase) {

  if (phase != nullptr) {
    std::cout << "After " << phase << "...\n";
  }

  std::cout << "     ";
  for (unsigned i = 0; i < 9; ++i) {
    std::cout << i << "   ";
  }
  std::cout << "\n";

  printLine(grid, 0, /*big_grid*/ true, /*thick*/ true, /*top*/ true,
            /*bottom*/ false, use_colour);
  std::cout << "\n";

  for (unsigned row = 0; row < 9; ++row) {
    const bool in_small_grid = row >= 3 && row <= 5;

    unsigned small_row = (row - 3) * 3;

    printRow((*grid)[row], row, 0, /*big_grid*/ true, use_colour);
    if (in_small_grid) {
      printRow((*grid)[small_row], small_row, 0, /*big_grid*/ false,
               use_colour);
    }
    std::cout << "\n";

    printRow((*grid)[row], row, 1, /*big_grid*/ true, use_colour);
    if (in_small_grid) {
      printRow((*grid)[small_row + 1], small_row + 1, 0, /*big_grid*/ false,
               use_colour);
    }
    std::cout << "\n";

    printRow((*grid)[row], row, 2, /*big_grid*/ true, use_colour);
    if (in_small_grid) {
      printRow((*grid)[small_row + 2], small_row + 2, 0, /*big_grid*/ false,
               use_colour);
    }
    std::cout << "\n";

    if (row != 8) {
      printLine(grid, row, /*big_grid*/ true, row == 2 || row == 5,
                /*top*/ false,
                /*bottom*/ false, use_colour);
      if (row == 2) {
        // The line just before the small grid starts
        printLine(grid, 0, /*big_grid*/ false, true, /*top*/ true,
                  /*bottom*/ false, use_colour);
      } else if (in_small_grid) {
        printLine(grid, small_row + 2, /*big_grid*/ false, /*thick*/ true,
                  /*top*/ false,
                  /*bottom*/ row == 5, use_colour);
      }
      std::cout << "\n";
    }
  }

  printLine(grid, 8, /*big_grid*/ true, /*thick*/ true, /*top*/ false,
            /*bottom*/ true, use_colour);
  std::cout << "\n";
}

int bitCount(const unsigned long mask) {
  int count = 0;
  for (int i = 0; i < 9; ++i) {
    count += ((mask >> i) & 0x1);
  }
  return count;
}

const char *getID(unsigned id) {
  switch (id) {
  default:
    return "X";
  case A:
    return "A";
  case B:
    return "B";
  case C:
    return "C";
  case D:
    return "D";
  case E:
    return "E";
  case F:
    return "F";
  case G:
    return "G";
  case H:
    return "H";
  case J:
    return "J";
  }
}

std::ostream &operator<<(std::ostream &os, const Coord &coord) {
  os << getID(coord.row) << coord.col;
  return os;
}
