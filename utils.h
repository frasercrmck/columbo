#ifndef COLUMBO_UTILS_H
#define COLUMBO_UTILS_H

#include "defs.h"

void generateSubsetSums(const unsigned target_sum,
                        const std::vector<IntList> &possibles,
                        std::vector<IntList> &subsets);

void printGrid(const Grid *const grid, bool use_colour,
               const char *phase = nullptr);

int bitCount(const Mask mask);

bool isOn(const Mask mask, unsigned x);

const char *getID(unsigned id);

std::string getHousePrintNum(House &house);

using CellCountMaskArray = std::array<Mask, 9>;

void collectCellCountMaskInfo(House &house, CellCountMaskArray &cell_masks);

#endif // COLUMBO_UTILS_H
