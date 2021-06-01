#ifndef COLUMBO_PRINTERS_TERMINAL_PRINTER_H
#define COLUMBO_PRINTERS_TERMINAL_PRINTER_H

#include <ostream>

struct Grid;

void printGrid(const Grid *const grid, std::ostream &os, bool use_colour,
               bool before = false, const char *phase = nullptr);

#endif // COLUMBO_PRINTERS_TERMINAL_PRINTER_H
