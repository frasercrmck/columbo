#ifndef COLUMBO_COMBINATIONS_H
#define COLUMBO_COMBINATIONS_H

#include "defs.h"
#include <vector>

void generateCageSubsetSums(const unsigned target_sum,
                            const std::vector<Mask> &possibles,
                            std::vector<IntList> &subsets);

void generateSubsetSumsWithDuplicates(const unsigned target_sum,
                                      const std::vector<Mask> &possibles,
                                      std::vector<IntList> &subsets);

#endif // COLUMBO_COMBINATIONS_H

