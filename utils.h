#ifndef SOLVER_UTILS_H
#define SOLVER_UTILS_H

#include "defs.h"

void generateSubsetSums(const int target_sum,
                        const std::vector<IntList> &possibles,
                        std::vector<IntList> &subsets);

void printGrid(const Grid *const grid, bool use_colour,
               const char *phase = nullptr);

int verify(Grid *grid, CageList &cages);

int bitCount(const Mask mask);

const char *getID(unsigned id);

std::string getHousePrintNum(House &house);

using CellCountMaskArray = std::array<Mask, 9>;

void collectCellCountMaskInfo(House &house, CellCountMaskArray &cell_masks);

#endif // SOLVER_UTILS_H
