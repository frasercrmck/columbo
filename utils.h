#ifndef SOLVER_UTILS_H
#define SOLVER_UTILS_H

#include "defs.h"

void subsetSum(const std::vector<int> &s, std::vector<int> &t,
               std::vector<std::vector<int>> &subsets, int sum, unsigned ite,
               const int target_sum, const unsigned subset_size);

void generateDefaultFixedSizeSubsets(const int target_sum,
                                     const unsigned subset_size,
                                     std::vector<std::vector<int>> &subsets);

void generateFixedSizeSubsets(const int target_sum, const unsigned subset_size,
                              const std::vector<int> &possibles,
                              std::vector<std::vector<int>> &subsets);

Cell *getCell(Grid *const grid, unsigned y, unsigned x);

Cell *getCell(Grid *const grid, const Coord &coord);

void printGrid(const Grid *const grid, const char *phase = nullptr);

int verify(Grid *grid, std::vector<Cage> &cages);

const char *getID(int id);

#endif // SOLVER_UTILS_H
