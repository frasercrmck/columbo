#ifndef COLUMBO_UTILS_H
#define COLUMBO_UTILS_H

#include "defs.h"

enum class Duplicates { No, Yes };

void generateSubsetSums(const unsigned target_sum,
                        const std::vector<IntList> &possibles,
                        const Duplicates allow_duplicates,
                        std::vector<IntList> &subsets);

void printGrid(const Grid *const grid, bool use_colour,
               const char *phase = nullptr);

const char *getID(unsigned id);

std::string getHousePrintNum(House &house);

using CellCountMaskArray = std::array<Mask, 9>;

CellCountMaskArray collectCellCountMaskInfo(const House &house);

#endif // COLUMBO_UTILS_H
