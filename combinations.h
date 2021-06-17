#ifndef COLUMBO_COMBINATIONS_H
#define COLUMBO_COMBINATIONS_H

#include "defs.h"
#include <vector>

std::vector<CageCombo>
generateCageSubsetSums(const unsigned target_sum,
                       const std::vector<Mask> &possibles);

void generateSubsetSumsWithDuplicates(const unsigned target_sum,
                                      const std::vector<Mask> &possibles,
                                      const std::vector<std::bitset<32>> &clashes,
                                      std::vector<IntList> &subsets);

void expandComboPermutations(Cage const *cage, CageCombo &cage_combo);

#endif // COLUMBO_COMBINATIONS_H

