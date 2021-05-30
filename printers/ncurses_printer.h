#ifndef COLUMBO_PRINTERS_NCURSES_PRINTER_H
#define COLUMBO_PRINTERS_NCURSES_PRINTER_H

#include "defs.h"

#include <cassert>
#include <optional>

struct CellBorders {
  bool up = true;
  bool down = true;
  bool left = true;
  bool right = true;
};

using CellBorderArray = std::array<std::array<CellBorders, 9>, 9>;

struct NCursesPrinter {
  bool initialize();
  void printGrid(int prow, int pcol, std::optional<Coord> cursor);

  void setGrid(const Grid *grid, const CellBorderArray *borders);

private:
  bool use_colour = true;
  const Grid *grid = nullptr;
  CellBorderArray *borders = nullptr;

  int printLine(const unsigned grid_row, int prow, int pcol,
                const bool big_grid, const bool thick, const bool top,
                const bool bottom);
  int printRow(House &house, unsigned row, unsigned sub_row, int prow, int pcol,
               bool big_grid, std::optional<Coord> cursor = std::nullopt);
};

#endif // COLUMBO_PRINTERS_NCURSES_PRINTER_H
