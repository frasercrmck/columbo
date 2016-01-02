#ifndef SOLVER_UTILS_H
#define SOLVER_UTILS_H

#include "defs.h"

void subsetSum(const IntList &s, IntList &t, std::vector<IntList> &subsets,
               int sum, unsigned ite, const int target_sum,
               const unsigned subset_size);

void generateDefaultFixedSizeSubsets(const int target_sum,
                                     const unsigned subset_size,
                                     std::vector<IntList> &subsets);

void generateFixedSizeSubsets(const int target_sum, const unsigned subset_size,
                              const IntList &possibles,
                              std::vector<IntList> &subsets);

Cell *getCell(Grid *const grid, unsigned y, unsigned x);

Cell *getCell(Grid *const grid, const Coord &coord);

void printGrid(const Grid *const grid, const char *phase = nullptr);

int verify(Grid *grid, CageList &cages);

const char *getID(int id);

#endif // SOLVER_UTILS_H
