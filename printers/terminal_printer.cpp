#include "../defs.h"
#include "../utils.h"
#include "terminal_printer.h"

#include <iostream>

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

void printLine(const Grid *const grid, std::ostream &os, const unsigned row,
               const bool big_grid, const bool thick, const bool top,
               const bool bottom, const bool use_colour) {
  // Indent three spaces
  os << "   ";

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
  os << beg_vert_glyph;

  for (unsigned x = 0; x < 9; ++x) {
    // Print three horizontal lines
    const Cell &this_cell = grid->cells[row][x];
    const bool down_is_cage_buddy =
        !top && row != 8 && this_cell.cage == grid->cells[row + 1][x].cage;
    const char *horiz_glyph = LIGHT_HORIZONTAL;
    if (down_is_cage_buddy && use_colour) {
      os << "\x1b[4" << this_cell.cage->colour << "m";
    }
    if (thick) {
      horiz_glyph = DOUBLE_HORIZONTAL;
    }
    // Small grid prints each cell 2 characters wide
    for (int z = 0; z < (big_grid ? 3 : 2); ++z) {
      os << horiz_glyph;
    }

    if (down_is_cage_buddy && use_colour) {
      // Reset attributes
      os << "\x1b[0m";
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
      os << intersect_glyph;
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
      os << intersect_glyph;
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

  os << end_vert_glyph;
}

void printBigGridCell(const Cell &cell, std::ostream &os, unsigned row,
                      bool use_colour) {
  if (use_colour) {
    // Coloured background
    os << "\x1b[4" << cell.cage->colour << "m";
    // Black foreground
    os << "\x1b[30m";
  }

  if (cell.isFixed()) {
    if (row != 1) {
      os << "   ";
    } else {
      os << " " << cell.isFixed() << " ";
    }
  } else {
    for (unsigned i = row * 3, e = i + 3; i < e; ++i) {
      if (cell.candidates[i]) {
        os << i + 1;
      } else {
        os << " ";
      }
    }
  }

  if (use_colour) {
    // Reset attributes
    os << "\x1b[0m";
  }
}

static void printSmallGridCell(const Cell &cell, std::ostream &os,
                               bool use_colour) {
  if (use_colour) {
    // Coloured background
    os << "\x1b[4" << cell.cage->colour << "m";
    // Black foreground
    os << "\x1b[30m";
  }

  Coord top_left = {9, 9};
  for (auto &other : *cell.cage) {
    if (other->coord.col < top_left.col && other->coord.row <= top_left.row)
      top_left = other->coord;
    if (other->coord.row < top_left.row)
      top_left = other->coord;
  }

  if (cell.coord == top_left) {
    if (cell.cage->sum < 10) {
      os << " ";
    }
    os << cell.cage->sum;
  } else {
    os << "  ";
  }

  if (use_colour) {
    // Reset attributes
    os << "\x1b[0m";
  }
}

void printRow(House &house, std::ostream &os, unsigned row, unsigned sub_row,
              bool big_grid, bool use_colour) {
  os << " ";
  if (sub_row == 1) {
    os << getRowID(row, USE_ROWCOL);
  } else {
    os << " ";
  }
  os << " ";
  os << DOUBLE_VERTICAL;
  for (unsigned col = 0; col < 9; ++col) {
    if (big_grid) {
      printBigGridCell(*house[col], os, sub_row, use_colour);
    } else {
      printSmallGridCell(*house[col], os, use_colour);
    }
    const Cell &this_cell = *house[col];
    const char *vert_glyph = LIGHT_DOUBLE_DASH_VERTICAL;
    const bool right_is_cage_buddy =
        col != 8 && this_cell.cage == house[col + 1]->cage;
    const bool is_thick = col == 2 || col == 5 || col == 8;
    if (right_is_cage_buddy && use_colour) {
      os << "\x1b[4" << this_cell.cage->colour << "m";
    }
    if (is_thick) {
      vert_glyph = DOUBLE_VERTICAL;
    } else {
      vert_glyph = LIGHT_VERTICAL;
    }
    os << vert_glyph;

    if (right_is_cage_buddy && use_colour) {
      // Reset attributes
      os << "\x1b[0m";
    }
  }
}

void printGrid(const Grid *const grid, std::ostream &os, bool use_colour,
               bool before, const char *phase) {

  if (phase != nullptr) {
    os << (before ? "Before " : "After ") << phase << "...\n";
  }

  os << "     ";
  for (unsigned i = 0; i < 9; ++i) {
    os << (i + 1) << "   ";
  }
  os << "\n";

  printLine(grid, os, 0, /*big_grid*/ true, /*thick*/ true, /*top*/ true,
            /*bottom*/ false, use_colour);
  os << "\n";

  for (unsigned row = 0; row < 9; ++row) {
    const bool in_small_grid = row >= 3 && row <= 5;

    unsigned small_row = (row - 3) * 3;

    printRow(*grid->rows[row], os, row, 0, /*big_grid*/ true, use_colour);
    if (in_small_grid) {
      printRow(*grid->rows[small_row], os, small_row, 0, /*big_grid*/ false,
               use_colour);
    }
    os << "\n";

    printRow(*grid->rows[row], os, row, 1, /*big_grid*/ true, use_colour);
    if (in_small_grid) {
      printRow(*grid->rows[small_row + 1], os, small_row + 1, 0,
               /*big_grid*/ false, use_colour);
    }
    os << "\n";

    printRow(*grid->rows[row], os, row, 2, /*big_grid*/ true, use_colour);
    if (in_small_grid) {
      printRow(*grid->rows[small_row + 2], os, small_row + 2, 0,
               /*big_grid*/ false, use_colour);
    }
    os << "\n";

    if (row != 8) {
      printLine(grid, os, row, /*big_grid*/ true, row == 2 || row == 5,
                /*top*/ false,
                /*bottom*/ false, use_colour);
      if (row == 2) {
        // The line just before the small grid starts
        printLine(grid, os, 0, /*big_grid*/ false, true, /*top*/ true,
                  /*bottom*/ false, use_colour);
      } else if (in_small_grid) {
        printLine(grid, os, small_row + 2, /*big_grid*/ false, /*thick*/ true,
                  /*top*/ false,
                  /*bottom*/ row == 5, use_colour);
      }
      os << "\n";
    }
  }

  printLine(grid, os, 8, /*big_grid*/ true, /*thick*/ true, /*top*/ false,
            /*bottom*/ true, use_colour);
  os << "\n";
}
