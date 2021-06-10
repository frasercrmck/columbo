#include "cli.h"
#include "defs.h"
#include "strategy.h"
#include "utils.h"
#include "printers/ncurses_printer.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>
#include <unordered_set>

#include <curses.h>

bool USE_COLOUR = true;

std::array<std::array<CellBorders, 9>, 9> borders;

std::unordered_set<Cell *> getReachableCells(Cell *cell, Grid &grid) {
  std::vector<Cell *> worklist{cell};
  std::unordered_set<Cell *> reachable{cell};
  while (!worklist.empty()) {
    auto *c = worklist.back();
    worklist.pop_back();

    auto coord = c->coord;

    if (!borders[coord.row][coord.col].up)
      if (auto *uc = grid.getCell(Coord{coord.row - 1, coord.col});
          reachable.insert(uc).second)
        worklist.push_back(uc);
    if (!borders[coord.row][coord.col].down)
      if (auto *dc = grid.getCell(Coord{coord.row + 1, coord.col});
          reachable.insert(dc).second)
        worklist.push_back(dc);
    if (!borders[coord.row][coord.col].left)
      if (auto *lc = grid.getCell(Coord{coord.row, coord.col - 1});
          reachable.insert(lc).second)
        worklist.push_back(lc);
    if (!borders[coord.row][coord.col].right)
      if (auto *rc = grid.getCell(Coord{coord.row, coord.col + 1});
          reachable.insert(rc).second)
        worklist.push_back(rc);
  }

  return reachable;
};

int main(int argc, char *argv[0]) {
  // Initialize curses
  setlocale(LC_CTYPE, "");
  initscr();
  cbreak();
  noecho();

  if (!has_colors())
    return 1;

  if (start_color() != OK)
    return 1;

  init_pair(1, COLOR_BLACK, COLOR_RED);
  init_pair(2, COLOR_BLACK, COLOR_BLUE);
  init_pair(3, COLOR_BLACK, COLOR_GREEN);
  init_pair(4, COLOR_BLACK, COLOR_YELLOW);
  init_pair(5, COLOR_BLACK, COLOR_MAGENTA);
  init_pair(6, COLOR_BLACK, COLOR_CYAN);

  clear();

  int max_cols = COLS - 1;
  int max_rows = LINES - 1;

  const char *file_name = nullptr;
  const char *out_file_name = nullptr;

  for (int i = 1; i < argc; ++i) {
    const char *opt = argv[i];
    if (isOpt(opt, "", "--no-colour")) {
      USE_COLOUR = false;
    } else if (isOpt(opt, "-f", "--file")) {
      if (i + 1 >= argc) {
        std::cerr << "Expected a value to option '" << opt << "'...\n";
        return 1;
      }
      file_name = argv[i + 1];
      ++i;
    } else if (isOpt(opt, "-o", "--out")) {
      if (i + 1 >= argc) {
        std::cerr << "Expected a value to option '" << opt << "'...\n";
        return 1;
      }
      out_file_name = argv[i + 1];
      ++i;
    }
  }

  auto grid = std::make_unique<Grid>();
  if (file_name) {
    std::ifstream sudoku_file;
    sudoku_file.open(file_name);

    if (!sudoku_file.is_open()) {
      std::cerr << "Could not open file '" << file_name << "'...\n";
      return 1;
    }

    if (grid->initialize(sudoku_file, false)) {
      std::cerr << "Invalid grid...\n";
      return 1;
    }
    sudoku_file.close();

  } else {
    for (unsigned y = 0; y < 9; y++) {
      for (unsigned x = 0; x < 9; x++) {
        auto cage = std::make_unique<Cage>(0u);
        cage->addCells(grid.get(), {grid->cells[y][x].coord});
        grid->cages.push_back(std::move(cage));
      }
    }

    for (auto &cage : grid->cages)
      for (auto &cell : cage->cells)
        cell->cage = cage.get();

    grid->assignCageColours();
  }

  // Assign cell borders for the flood-filling algorithms.
  for (unsigned y = 0; y < 9; y++) {
    for (unsigned x = 0; x < 9; x++) {
      auto *cell = grid->getCell(y, x);
      if (x != 0 && cell->cage == grid->getCell(y, x - 1)->cage) {
        borders[y][x].left = false;
        borders[y][x - 1].right = false;
      }
      if (x != 8 && cell->cage == grid->getCell(y, x + 1)->cage) {
        borders[y][x].right = false;
        borders[y][x + 1].left = false;
      }
      if (y != 0 && cell->cage == grid->getCell(y - 1, x)->cage) {
        borders[y][x].up = false;
        borders[y - 1][x].down = false;
      }
      if (y != 8 && cell->cage == grid->getCell(y + 1, x)->cage) {
        borders[y][x].down = false;
        borders[y + 1][x].up = false;
      }
    }
  }

  Coord cursor{0, 0};

  bool sum_mode = false;
  bool candidate_mode = false;

  NCursesPrinter printer;

  if (printer.initialize()) {
    return 1;
  }

  printer.setGrid(grid.get(), &borders);

  while (true) {
    printer.printGrid(0, 7, cursor);

    int help_row = max_rows - 10;
    int help_col = 0;
    mvaddstr(help_row++, help_col, "HELP: hjkl: move cursor left/down/up/right");
    mvaddstr(help_row++, help_col,
             "      HJKL: toggle cell border between cursor cell and cursor "
             "left/down/up/right");
    mvaddstr(help_row++, help_col,
             "      s: toggle cage sum mode, clear cage sum (hjkl/HJKL/c exits)");
    mvaddstr(help_row++, help_col,
             "         0-9: enter cage sum");
    mvaddstr(help_row++, help_col,
             "      c: toggle cell-candidate mode           (hjkl/HJKL/s exits)");
    mvaddstr(help_row++, help_col, "         1-9: toggle cell candidate");
    mvaddstr(
        help_row++, help_col,
        "      x: invert all cell candidates and enter cell-candidate mode");

    int mode_row = max_rows - 1;
    int mode_col = 0;

    mvaddstr(mode_row, mode_col, "MODE: ");
    mode_col += 6;

    if (sum_mode)
      mvaddstr(mode_row, mode_col, "CAGE SUM ");
    else if (candidate_mode)
      mvaddstr(mode_row, mode_col, "CELL CAND");
    else
      mvaddstr(mode_row, mode_col, "DEFAULT  ");
    refresh();

    int c = getch();
    if (c == 'q')
      break;

    Cell *old_cell = grid->getCell(cursor);
    Cage *old_cage = old_cell->cage;

    if (c == 'I') {
      cursor.col = 0;
      continue;
    }
    if (c == 'A') {
      cursor.col = 8;
      continue;
    }

    if (c == 'c') {
      candidate_mode ^= true;
      sum_mode = false;
      continue;
    }

    if (c == 'x') {
      old_cell->candidates.flip();
      candidate_mode = true;
      sum_mode = false;
      continue;
    }

    if (c == 's') {
      sum_mode ^= true;
      candidate_mode = false;
      if (sum_mode)
        old_cage->sum = 0;
      continue;
    }

    if (c >= '0' && c <= '9') {
      int val = c - '0';
      if (sum_mode)
        old_cage->sum = old_cage->sum * 10 + val;
      else if (candidate_mode && val != 0) {
        old_cell->candidates[static_cast<size_t>(val - 1)].flip();
      }
      continue;
    }

    sum_mode = false;
    candidate_mode = false;

    Coord new_cursor = cursor;
    if (c == 'h' || c == 'H')
      new_cursor.col = (unsigned)std::max(0, (int)new_cursor.col - 1);
    if (c == 'j' || c == 'J')
      new_cursor.row = (unsigned)std::min(8, (int)new_cursor.row + 1);
    if (c == 'k' || c == 'K')
      new_cursor.row = (unsigned)std::max(0, (int)new_cursor.row - 1);
    if (c == 'l' || c == 'L')
      new_cursor.col = (unsigned)std::min(8, (int)new_cursor.col + 1);

    if (cursor == new_cursor)
      continue;

    if (c == 'h' || c == 'j' || c == 'k' || c == 'l') {
      cursor = new_cursor;
      continue;
    }

    auto &cursor_borders = borders[cursor.row][cursor.col];
    auto &new_cursor_borders = borders[new_cursor.row][new_cursor.col];

    if (c == 'H') {
      cursor_borders.left ^= 1;
      new_cursor_borders.right ^= 1;
    } else if (c == 'J') {
      cursor_borders.down ^= 1;
      new_cursor_borders.up ^= 1;
    } else if (c == 'K') {
      cursor_borders.up ^= 1;
      new_cursor_borders.down ^= 1;
    } else if (c == 'L') {
      cursor_borders.right ^= 1;
      new_cursor_borders.left ^= 1;
    }

    Cell *new_cell = grid->getCell(new_cursor);
    auto it = std::find_if(
        std::begin(grid->cages), std::end(grid->cages),
        [new_cell](const std::unique_ptr<Cage> &cage) {
          return std::find(std::begin(cage->cells), std::end(cage->cells),
                           new_cell) != std::end(cage->cells);
        });
    assert(it != std::end(grid->cages));

    Cage *new_cage = (*it).get();

    if ((!cursor_borders.down || !cursor_borders.left ||
         !cursor_borders.right || !cursor_borders.up) &&
        old_cage != new_cage) {
      // Merge cages
      for (auto *c : new_cage->cells) {
        old_cage->addCell(grid.get(), c->coord);
        c->cage = old_cage;
      }
      grid->cages.erase(it);
    } else if (old_cage == new_cage) {
      auto old_cage_cells = getReachableCells(old_cell, *grid);
      auto new_cage_cells = getReachableCells(new_cell, *grid);
      if (old_cage_cells.size() != new_cage_cells.size() ||
          old_cage_cells.size() != old_cage->size()) {
        // Create a new cage for the 'new' cells
        auto cage = std::make_unique<Cage>(0u);
        for (auto *c : new_cage_cells) {
          cage->addCell(grid.get(), c->coord);
          grid->getCell(c->coord)->cage = cage.get();
        }
        // Erase the 'new' cells from the 'old' cage
        old_cage->cells.erase(
            std::remove_if(std::begin(*old_cage), std::end(*old_cage),
                           [&new_cage_cells](Cell *c) {
                             return new_cage_cells.count(c) != 0;
                           }),
            std::end(*old_cage));
        // Push back this new cage
        grid->cages.push_back(std::move(cage));
      }
    }

    grid->assignCageColours();
    cursor = new_cursor;
  }

  if (out_file_name) {
    std::ofstream out_file;
    out_file.open(out_file_name);

    grid->writeToFile(out_file);
  }

  endwin();

  return 0;
}
