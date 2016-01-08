#ifndef SOLVER_UTILS_H
#define SOLVER_UTILS_H

#include "defs.h"

void generateSubsetSums(const int target_sum,
                        const std::vector<IntList> &possibles,
                        std::vector<IntList> &subsets);

Cell *getCell(Grid *const grid, unsigned y, unsigned x);

Cell *getCell(Grid *const grid, const Coord &coord);

void printGrid(const Grid *const grid, bool use_colour,
               const char *phase = nullptr);

int verify(Grid *grid, CageList &cages);

int bitCount(const unsigned long mask);

const char *getID(unsigned id);

#endif // SOLVER_UTILS_H
