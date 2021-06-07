#ifndef COLUMBO_COMBINATIONS_H
#define COLUMBO_COMBINATIONS_H

#include "defs.h"
#include <vector>

enum class Duplicates { No, Yes };

void generateSubsetSums(const unsigned target_sum,
                        const std::vector<Mask> &possibles,
                        const Duplicates allow_duplicates,
                        std::vector<IntList> &subsets);

#endif // COLUMBO_COMBINATIONS_H

