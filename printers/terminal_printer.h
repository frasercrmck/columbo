#ifndef COLUMBO_PRINTERS_TERMINAL_PRINTER_H
#define COLUMBO_PRINTERS_TERMINAL_PRINTER_H

struct Grid;

void printGrid(const Grid *const grid, bool use_colour,
               const char *phase = nullptr);

#endif // COLUMBO_PRINTERS_TERMINAL_PRINTER_H
