#ifndef COLUMBO_COMBINATIONS_H
#define COLUMBO_COMBINATIONS_H

#include "defs.h"
#include <vector>

std::vector<CageCombo>
generateCageSubsetSums(const unsigned target_sum,
                       const std::vector<Mask> &possibles);

std::vector<CageCombo>
generateSubsetSumsWithDuplicates(const unsigned target_sum,
                                 const std::vector<Mask> &possibles,
                                 const std::vector<CellMask> &clashes);

void expandComboPermutations(Cage const *cage, CageCombo &cage_combo);

bool reduceBasedOnCageRelations(Cage &lhs, Cage &rhs, int sum, CellSet &changed,
                                bool debug, std::string const &debug_banner);

template <typename T>
void remove_by_indices(std::vector<T> &vec,
                       std::vector<std::size_t> const &remove_indices) {
  auto vec_base = std::begin(vec);
  typename std::vector<T>::size_type down_by = 0;
  for (auto it = remove_indices.cbegin(), ei = remove_indices.cend(); it != ei;
       it++, down_by++) {
    typename std::vector<T>::size_type next =
        (it + 1 == ei) ? vec.size() : *(it + 1);
    std::move(vec_base + *it + 1, vec_base + next, vec_base + *it - down_by);
  }
  vec.resize(vec.size() - remove_indices.size());
}

#endif // COLUMBO_COMBINATIONS_H

