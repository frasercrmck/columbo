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

bool reduceBasedOnCageRelations(Cage &lhs, Cage &rhs, int sum, CellSet &changed,
                                bool debug, std::string const &debug_banner);

#endif // COLUMBO_COMBINATIONS_H

