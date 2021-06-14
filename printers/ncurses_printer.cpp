#include "ncurses_printer.h"
#include "utils.h"

#include <curses.h>

#define BOX_DRAWING_CHARS                                                      \
  X(LIGHT_DOUBLE_DASH_HORIZONTAL, L"\u254c")                                   \
  X(LIGHT_DOUBLE_DASH_VERTICAL, L"\254e")                                      \
  X(DOUBLE_HORIZONTAL, L"\u2550")                                              \
  X(DOUBLE_VERTICAL, L"\u2551")                                                \
  X(DOUBLE_DOWN_AND_RIGHT, L"\u2554")                                          \
  X(DOUBLE_DOWN_AND_LEFT, L"\u2557")                                           \
  X(DOUBLE_UP_AND_RIGHT, L"\u255a")                                            \
  X(DOUBLE_UP_AND_LEFT, L"\u255d")                                             \
  X(DOUBLE_VERTICAL_AND_RIGHT, L"\u2560")                                      \
  X(DOUBLE_VERTICAL_AND_LEFT, L"\u2563")                                       \
  X(VERTICAL_DOUBLE_AND_RIGHT_SINGLE, L"\u255f")                               \
  X(VERTICAL_DOUBLE_AND_LEFT_SINGLE, L"\u2562")                                \
  X(DOUBLE_DOWN_AND_HORIZONTAL, L"\u2566")                                     \
  X(DOUBLE_UP_AND_HORIZONTAL, L"\u2569")                                       \
  X(DOUBLE_VERTICAL_AND_HORIZONTAL, L"\u256c")                                 \
  X(VERTICAL_SINGLE_AND_HORIZONTAL_DOUBLE, L"\u256a")                          \
  X(VERTICAL_DOUBLE_AND_HORIZONTAL_SINGLE, L"\u256b")                          \
  X(DOWN_SINGLE_AND_HORIZONTAL_DOUBLE, L"\u2564")                              \
  X(UP_SINGLE_AND_HORIZONTAL_DOUBLE, L"\u2567")                                \
  X(LIGHT_VERTICAL_AND_HORIZONTAL, L"\u253c")                                  \
  X(LIGHT_UP, L"\u2575")                                                       \
  X(LIGHT_LEFT, L"\u2574")                                                     \
  X(LIGHT_VERTICAL, L"\u2502")                                                 \
  X(LIGHT_HORIZONTAL, L"\u2500")

enum {
#define X(ID, STR) ID,
  BOX_DRAWING_CHARS
#undef X
      NUM,
};

static cchar_t box_chars[NUM];

static void printBoxChar(int prow, int pcol, unsigned char_idx) {
  mvadd_wch(prow, pcol, &box_chars[char_idx]);
}

static int printBigGridCell(const Cell &cell, unsigned sub_row, int prow, int pcol,
                            bool use_colour,
                            std::optional<Coord> cursor = std::nullopt) {
  if (use_colour)
    attrset(COLOR_PAIR(cell.cage->colour));

  if (cell.coord == cursor)
    attron(WA_BLINK);

  if (cell.isFixed()) {
    mvaddch(prow, pcol++, ' ');
    if (sub_row != 1)
      mvaddch(prow, pcol++, ' ');
    else
      mvaddch(prow, pcol++, std::to_string(cell.isFixed())[0]);
    mvaddch(prow, pcol++, ' ');
  } else {
    for (unsigned i = sub_row * 3, e = i + 3; i < e; ++i) {
      if (cell.candidates[i])
        mvaddch(prow, pcol++, std::to_string(i + 1)[0]);
      else
        mvaddch(prow, pcol++, ' ');
    }
  }

  if (use_colour)
    attroff(COLOR_PAIR(cell.cage->colour));

  if (cell.coord == cursor)
    attroff(WA_BLINK);

  return pcol;
}

static int printSmallGridCell(const Cell &cell, int prow, int pcol,
                              bool use_colour,
                              std::optional<Coord> cursor = std::nullopt) {
  if (use_colour)
    attron(COLOR_PAIR(cell.cage->colour));

  Coord top_left = {9, 9};
  for (auto &other : *cell.cage) {
    if (other->coord.col < top_left.col && other->coord.row <= top_left.row)
      top_left = other->coord;
    if (other->coord.row < top_left.row)
      top_left = other->coord;
  }

  if (cell.coord == cursor)
    attron(WA_BLINK);

  if (cell.coord == top_left) {
    if (cell.cage->sum < 10)
      mvaddch(prow, pcol++, ' ');
    mvaddch(prow, pcol++, std::to_string(cell.cage->sum)[0]);
    if (cell.cage->sum >= 10)
      mvaddch(prow, pcol++, std::to_string(cell.cage->sum)[1]);
  } else if (cell.coord == cursor) {
    mvaddch(prow, pcol++, ' ');
    mvaddch(prow, pcol++, 'x');
  } else {
    mvaddch(prow, pcol++, ' ');
    mvaddch(prow, pcol++, ' ');
  }

  // Reset attributes
  if (use_colour)
    attroff(COLOR_PAIR(cell.cage->colour));

  if (cell.coord == cursor)
    attroff(WA_BLINK);

  return pcol;
}

int NCursesPrinter::printRow(House &house, unsigned row, unsigned sub_row,
                             int prow, int pcol, bool big_grid,
                             std::optional<Coord> cursor) {
  if (sub_row == 1) {
    mvaddch(prow, pcol, *getRowID(row, USE_ROWCOL));
  }
  pcol++;
  printBoxChar(prow, pcol++, DOUBLE_VERTICAL);

  for (unsigned col = 0; col < 9; ++col) {
    if (big_grid) {
      pcol = printBigGridCell(*house[col], sub_row, prow, pcol, use_colour,
                              cursor);
    } else {
      pcol = printSmallGridCell(*house[col], prow, pcol, use_colour, cursor);
    }
    const Cell &this_cell = *house[col];
    unsigned vert_glyph = LIGHT_DOUBLE_DASH_VERTICAL;
    const bool right_is_cage_buddy =
        col != 8 && this_cell.cage == house[col + 1]->cage;
    const bool is_thick = col == 2 || col == 5 || col == 8;
    if (right_is_cage_buddy && use_colour)
      attrset(COLOR_PAIR(this_cell.cage->colour));
    if (is_thick)
      vert_glyph = DOUBLE_VERTICAL;
    else
      vert_glyph = LIGHT_VERTICAL;
    if (big_grid && right_is_cage_buddy && borders &&
        !(*borders)[this_cell.coord.row][this_cell.coord.col].right)
      mvaddch(prow, pcol++, ' ');
    else
      printBoxChar(prow, pcol++, vert_glyph);

    if (right_is_cage_buddy && use_colour)
      attroff(COLOR_PAIR(this_cell.cage->colour));
  }

  return pcol;
}

int NCursesPrinter::printLine(const unsigned grid_row, int prow, int pcol,
                              const bool big_grid, const bool thick,
                              const bool top, const bool bottom) {
  pcol++;
  // Print the initial box boundary
  unsigned beg_char_idx = VERTICAL_DOUBLE_AND_RIGHT_SINGLE;
  if (thick) {
    if (top) {
      beg_char_idx = DOUBLE_DOWN_AND_RIGHT;
    } else if (bottom) {
      beg_char_idx = DOUBLE_UP_AND_RIGHT;
    } else {
      beg_char_idx = DOUBLE_VERTICAL_AND_RIGHT;
    }
  }

  printBoxChar(prow, pcol++, beg_char_idx);

  for (unsigned x = 0; x < 9; ++x) {
    // Print three horizontal lines
    const Cell &this_cell = grid->cells[grid_row][x];
    const bool down_is_cage_buddy =
        !top && grid_row != 8 &&
        this_cell.cage == grid->cells[grid_row + 1][x].cage;
    unsigned horiz_glyph = LIGHT_HORIZONTAL;
    if (down_is_cage_buddy && use_colour)
      attrset(COLOR_PAIR(this_cell.cage->colour));
    if (thick)
      horiz_glyph = DOUBLE_HORIZONTAL;
    // Small grid prints each cell 2 characters wide
    if (big_grid && down_is_cage_buddy && borders &&
        !(*borders)[this_cell.coord.row][this_cell.coord.col].down)
      for (int z = 0; z < (big_grid ? 3 : 2); ++z)
        mvaddch(prow, pcol++, ' ');
    else
      for (int z = 0; z < (big_grid ? 3 : 2); ++z)
        printBoxChar(prow, pcol++, horiz_glyph);

    // Reset attributes
    if (down_is_cage_buddy && use_colour)
      attroff(COLOR_PAIR(this_cell.cage->colour));

    unsigned intersect_glyph = 0;
    if (x == 8)
      continue;

    if (x == 2 || x == 5) {
      // If we're at a box intersection...
      intersect_glyph = VERTICAL_DOUBLE_AND_HORIZONTAL_SINGLE;
      if (thick) {
        if (top)
          intersect_glyph = DOUBLE_DOWN_AND_HORIZONTAL;
        else if (bottom)
          intersect_glyph = DOUBLE_UP_AND_HORIZONTAL;
        else
          intersect_glyph = DOUBLE_VERTICAL_AND_HORIZONTAL;
      }
    } else {
      // If we're at a regular cell intersection...
      intersect_glyph = LIGHT_VERTICAL_AND_HORIZONTAL;
      if (thick) {
        if (top)
          intersect_glyph = DOWN_SINGLE_AND_HORIZONTAL_DOUBLE;
        else if (bottom)
          intersect_glyph = UP_SINGLE_AND_HORIZONTAL_DOUBLE;
        else
          intersect_glyph = VERTICAL_SINGLE_AND_HORIZONTAL_DOUBLE;
      }
    }
    printBoxChar(prow, pcol++, intersect_glyph);
  }

  // Print the end of the line: a vertical bar
  unsigned end_vert_glyph = VERTICAL_DOUBLE_AND_LEFT_SINGLE;

  if (thick) {
    if (top) {
      end_vert_glyph = DOUBLE_DOWN_AND_LEFT;
    } else if (bottom) {
      end_vert_glyph = DOUBLE_UP_AND_LEFT;
    } else {
      end_vert_glyph = DOUBLE_VERTICAL_AND_LEFT;
    }
  }

  printBoxChar(prow, pcol++, end_vert_glyph);

  return pcol;
}

bool NCursesPrinter::initialize() {
  std::pair<unsigned, const wchar_t *> chars[] = {
#define X(IDX, STR) {IDX, STR},
      BOX_DRAWING_CHARS
#undef X
  };
  for (const auto &[idx, str] : chars)
    setcchar(&box_chars[idx], str, WA_NORMAL, COLOR_PAIR(0), NULL);

  return false;
}

void NCursesPrinter::setGrid(const Grid *g, const CellBorderArray *bdrs) {
  grid = g;
  borders = bdrs;
}

void NCursesPrinter::printGrid(int prow, int pcol,
                               std::optional<Coord> cursor) {
  for (unsigned i = 0; i < 9; ++i) {
    mvaddch(prow, pcol + i * 4, std::to_string(i)[0]);
  }

  prow++;
  pcol = 4;
  int first_pcol = 4;

  printLine(0, prow++, pcol, /*big_grid*/ true, /*thick*/ true,
            /*top*/ true,
            /*bottom*/ false);

  for (unsigned grid_row = 0; grid_row < 9; ++grid_row) {
    int new_pcol = 0;
    const bool in_small_grid = grid_row >= 3 && grid_row <= 5;

    unsigned small_row = (grid_row - 3) * 3;

    new_pcol = printRow(*grid->rows[grid_row], grid_row, 0, prow, pcol,
                        /*big_grid*/ true, cursor);
    if (in_small_grid) {
      printRow(*grid->rows[small_row], small_row, 0, prow, new_pcol,
               /*big_grid*/ false);
    }
    prow++;

    new_pcol = printRow(*grid->rows[grid_row], grid_row, 1, prow, pcol,
                        /*big_grid*/ true, cursor);
    if (in_small_grid) {
      printRow(*grid->rows[small_row + 1], small_row + 1, 0, prow, new_pcol,
               /*big_grid*/ false, cursor);
    }
    prow++;

    new_pcol = printRow(*grid->rows[grid_row], grid_row, 2, prow, pcol,
                        /*big_grid*/ true, cursor);
    if (in_small_grid) {
      printRow(*grid->rows[small_row + 2], small_row + 2, 0, prow, new_pcol,
               /*big_grid*/ false, cursor);
    }
    prow++;

    if (grid_row != 8) {
      new_pcol = printLine(grid_row, prow, pcol, /*big_grid*/ true,
                           grid_row == 2 || grid_row == 5,
                           /*top*/ false,
                           /*bottom*/ false);
      if (grid_row == 2) {
        // The line just before the small grid starts
        printLine(0, prow, new_pcol, /*big_grid*/ false, true,
                  /*top*/ true,
                  /*bottom*/ false);
      } else if (in_small_grid) {
        printLine(small_row + 2, prow, new_pcol, /*big_grid*/ false,
                  /*thick*/ true,
                  /*top*/ false,
                  /*bottom*/ grid_row == 5);
      }
      prow++;
    }
  }

  printLine(8, prow, pcol, /*big_grid*/ true, /*thick*/ true,
            /*top*/ false,
            /*bottom*/ true);
}
