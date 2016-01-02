#include "utils.h"

#include <iostream>

void subsetSum(const IntList &possible_values, IntList &subset,
               std::vector<IntList> &subsets, const int current_sum,
               const unsigned current_idx, const int target_sum,
               const unsigned subset_size) {
  if (target_sum == current_sum) {
    if (subset.size() == subset_size) {
      subsets.push_back(subset);
    }

    // Exclude the previously-added item and consider the next candidate
    const int last = subset.back();
    subset.pop_back();

    subsetSum(possible_values, subset, subsets, current_sum - last,
              current_idx + 1, target_sum, subset_size);
    return;
  } else {
    // Generate nodes along the breadth
    for (unsigned i = current_idx; i < possible_values.size(); i++) {
      IntList local_subset = subset;
      local_subset.push_back(possible_values[i]);
      // Consider the next level node along the depth
      subsetSum(possible_values, local_subset, subsets,
                current_sum + possible_values[i], i + 1, target_sum,
                subset_size);
    }
  }
}

void generateDefaultFixedSizeSubsets(const int target_sum,
                                     const unsigned subset_size,
                                     std::vector<IntList> &subsets) {
  const IntList all_numbers = {1, 2, 3, 4, 5, 6, 7, 8, 9};
  return generateFixedSizeSubsets(target_sum, subset_size, all_numbers,
                                  subsets);
}

void generateFixedSizeSubsets(const int target_sum, const unsigned subset_size,
                              const IntList &possibles,
                              std::vector<IntList> &subsets) {
  int current_sum = 0;
  unsigned current_idx = 0;
  IntList tuple;
  subsetSum(possibles, tuple, subsets, current_sum, current_idx, target_sum,
            subset_size);
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

void printLine(const Grid *const grid, const unsigned row, const bool thick,
               const bool top, const bool bottom) {
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
    const bool down_is_cage_buddy =
        !top && row != 8 && (*grid)[row][x].cage == (*grid)[row + 1][x].cage;
    const char *horiz_glyph = LIGHT_HORIZONTAL;
    if (down_is_cage_buddy) {
      // Can do something with this eventually
      horiz_glyph = thick ? DOUBLE_HORIZONTAL : LIGHT_HORIZONTAL;
    } else if (thick) {
      horiz_glyph = DOUBLE_HORIZONTAL;
    }
    for (int z = 0; z < 3; ++z) {
      std::cout << horiz_glyph;
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

  std::cout << end_vert_glyph << "\n";
}

void printCandidates(const Cell &cell, int row) {
  if (cell.isFixed() && row != 1) {
    std::cout << "   ";
    return;
  }

  if (cell.isFixed()) {
    unsigned fixed = cell.isFixed();
    std::cout << " " << fixed << " ";
    return;
  }

  for (int i = row * 3, e = i + 3; i < e; ++i) {
    if (cell.candidates[static_cast<std::size_t>(i)]) {
      std::cout << i + 1;
    } else {
      std::cout << " ";
    }
  }
}

void printRow(const std::array<Cell, 9> &house, unsigned row, int sub_row) {
  std::cout << " ";
  if (sub_row == 1) {
    std::cout << getID(static_cast<int>(row));
  } else {
    std::cout << " ";
  }
  std::cout << " ";
  std::cout << DOUBLE_VERTICAL;
  for (unsigned col = 0; col < 9; ++col) {
    printCandidates(house[col], sub_row);
    const char *vert_glyph = LIGHT_DOUBLE_DASH_VERTICAL;
    const bool right_is_cage_buddy =
        col != 8 && house[col].cage == house[col + 1].cage;
    const bool is_thick = col == 2 || col == 5 || col == 8;
    if (right_is_cage_buddy) {
      // Can do something with this eventually
      vert_glyph = is_thick ? DOUBLE_VERTICAL : LIGHT_VERTICAL;
    } else if (is_thick) {
      vert_glyph = DOUBLE_VERTICAL;
    } else {
      vert_glyph = LIGHT_VERTICAL;
    }
    std::cout << vert_glyph;
  }
  std::cout << "\n";
}

void printGrid(const Grid *const grid, const char *phase) {

  if (phase != nullptr) {
    std::cout << "After " << phase << "...\n";
  }

  std::cout << "     ";
  for (unsigned i = 0; i < 9; ++i) {
    std::cout << i + 1 << "   ";
  }
  std::cout << "\n";

  printLine(grid, 0, /*thick*/ true, /*top*/ true, /*bottom*/ false);

  for (unsigned row = 0; row < 9; ++row) {
    printRow((*grid)[row], row, 0);
    printRow((*grid)[row], row, 1);
    printRow((*grid)[row], row, 2);
    if (row != 8) {
      printLine(grid, row, row == 2 || row == 5, /*top*/ false,
                /*bottom*/ false);
    }
  }

  printLine(grid, 8, /*thick*/ true, /*top*/ false, /*bottom*/ true);
}

const char *getID(int id) {
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
