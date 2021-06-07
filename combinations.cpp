#include "combinations.h"
#include <algorithm>
#include <numeric>
#include <cassert>
#include <iostream>
#include <unordered_set>

static unsigned max_value(Mask m) {
  assert(m.any() && "Unset mask");
  for (int i = static_cast<int>(m.size()) - 1; i >= 0; --i) {
    if (m[static_cast<unsigned>(i)])
      return static_cast<unsigned>(i + 1);
  }
  return 0;
}

static unsigned min_value(Mask m) {
  assert(m.any() && "Unset mask");
  for (unsigned i = 0, e = m.size(); i != e; ++i) {
    if (m[static_cast<unsigned>(i)])
      return static_cast<unsigned>(i + 1);
  }
  return 0;
}

// Given a list of lists of possible cage values:
//     [{1,2,3}, {3,4,5}]
// Recursively generates tuples of combinations from each of the lists as
// follows:
//   [1,3]
//   [1,4]
//   [1,5]
//   [2,3]
//   [2,4]
// ... etc
// Each of these is checked against the target sum, and pushed into a result
// vector if they match.
// Note: The algorithm assumes that the list of possibles/candidates are
// ordered. This allows it to bail out early if it detects there's no point
// going further.
static void subsetSum(const std::vector<Mask> &possible_lists,
                      Mask const current_combo, unsigned tuple_sum,
                      std::vector<CageCombo> &subsets,
                      const unsigned target_sum, unsigned list_idx) {
  std::size_t const p_size = possible_lists.size();
  std::size_t const m_size = possible_lists[0].size();
  for (unsigned p = list_idx; p < p_size; ++p) {
    for (unsigned i = 0; i != m_size; ++i) {
      if (!possible_lists[p][i])
        continue;
      // Can't repeat a value inside a cage
      if (current_combo[i])
        continue;

      auto poss = i + 1;
      // Optimization for small target sums: if the candidate is bigger than
      // the target itself then it can't be valid, neither can any candidate
      // after it (ordered).
      if (target_sum < static_cast<unsigned>(poss)) {
        break;
      }

      // If the running total plus all the minimums of the remaining candidates
      // is guaranteed to exceed the sum, bail out here (ordered).
      if (std::accumulate(possible_lists.begin() + p + 1, possible_lists.end(),
                          tuple_sum + poss, [](const unsigned A, const Mask B) {
                            return A + min_value(B);
                          }) > target_sum) {
        break;
      }

      // If the running total plus all the maximums of the remaining candidates
      // is insufficient to reach the target, skip this candidate and try a
      // larger one.
      if (std::accumulate(possible_lists.begin() + p + 1, possible_lists.end(),
                          tuple_sum + poss, [](const unsigned A, const Mask B) {
                            return A + max_value(B);
                          }) < target_sum) {
        continue;
      }

      // Pre-calculate the new tuple values to avoid spurious
      // insertions/deletions to the vector.
      const auto new_tuple_sum = tuple_sum + poss;
      const auto new_tuple_size = current_combo.count() + 1;

      // If we've added too much then we can bail out (ordered).
      if (new_tuple_sum > target_sum) {
        break;
      }

      // If there are fewer spots left in the tuple than there are options for
      // the sum to reach the target, bail.
      // TODO: This could be more sophisticated (can't have more than one 1, so
      // it's more like the N-1 sum that it should be greater than.
      if ((p_size - new_tuple_size) > (target_sum - new_tuple_sum))
        break;

      if (new_tuple_size == p_size) {
        // If we've reached our target size then we can stop searching other
        // possiblities from this list (ordered).
        auto m = current_combo | Mask(1 << i);
        if (new_tuple_sum == target_sum &&
            std::find_if(std::begin(subsets), std::end(subsets),
                         [m](CageCombo const &c) { return c.combo == m; }) ==
                std::end(subsets)) {
          subsets.push_back(CageCombo{m});
          break;
        }

        // Else, move on to the next candidate in the list.
        continue;
      }

      tuple_sum += poss;
      subsetSum(possible_lists, current_combo | Mask(1 << i), tuple_sum,
                subsets, target_sum, p + 1);
      tuple_sum -= poss;
    }
  }
}

std::vector<CageCombo>
generateCageSubsetSums(const unsigned target_sum,
                       const std::vector<Mask> &possibles) {
  Mask current_combo;
  std::vector<CageCombo> subsets;
  subsetSum(possibles, current_combo, 0, subsets, target_sum, 0);
  return subsets;
}

static void subsetSumWithDuplicates(const std::vector<Mask> &possible_lists,
                                    IntList &tuple, unsigned tuple_sum,
                                    std::vector<IntList> &subsets,
                                    const unsigned target_sum,
                                    unsigned list_idx) {
  std::size_t const p_size = possible_lists.size();
  std::size_t const m_size = possible_lists[0].size();
  for (unsigned p = list_idx; p < p_size; ++p) {
    for (unsigned i = 0; i != m_size; ++i) {
      if (!possible_lists[p][i])
        continue;
      auto poss = i + 1;
      // Optimization for small target sums: if the candidate is bigger than
      // the target itself then it can't be valid, neither can any candidate
      // after it (ordered).
      if (target_sum < static_cast<unsigned>(poss)) {
        break;
      }

      // If the running total plus all the minimums of the remaining candidates is
      // guaranteed to exceed the sum, bail out here (ordered).
      if (std::accumulate(possible_lists.begin() + p + 1, possible_lists.end(),
                          tuple_sum + poss, [](const unsigned A, const Mask B) {
                            return A + min_value(B);
                          }) > target_sum) {
        break;
      }

      // If the running total plus all the maximums of the remaining candidates
      // is insufficient to reach the target, skip this candidate and try a
      // larger one.
      if (std::accumulate(possible_lists.begin() + p + 1, possible_lists.end(),
                          tuple_sum + poss, [](const unsigned A, const Mask B) {
                            return A + max_value(B);
                          }) < target_sum) {
        continue;
      }

      // Pre-calculate the new tuple values to avoid spurious
      // insertions/deletions to the vector.
      const auto new_tuple_sum = tuple_sum + poss;
      const auto new_tuple_size = tuple.size() + 1;

      // If we've added too much then we can bail out (ordered).
      if (new_tuple_sum > target_sum) {
        break;
      }

      // If there are fewer spots left in the tuple than there are options for
      // the sum to reach the target, bail.
      // TODO: This could be more sophisticated (can't have more than one 1, so
      // it's more like the N-1 sum that it should be greater than.
      if ((p_size - new_tuple_size) > (target_sum - new_tuple_sum)) {
        break;
      }

      if (new_tuple_size == p_size) {
        // If we've reached our target size then we can stop searching other
        // possiblities from this list (ordered).
        if (new_tuple_sum == target_sum) {
          tuple.push_back(poss);
          subsets.push_back(tuple);
          tuple.pop_back();
          break;
        }

        // Else, move on to the next candidate in the list.
        continue;
      }

      tuple_sum += poss;
      tuple.push_back(poss);

      subsetSumWithDuplicates(possible_lists, tuple, tuple_sum, subsets,
                              target_sum, p + 1);

      tuple.pop_back();
      tuple_sum -= poss;
    }
  }
}

void generateSubsetSumsWithDuplicates(const unsigned target_sum,
                                      const std::vector<Mask> &possibles,
                                      std::vector<IntList> &subsets) {
  IntList tuple;
  subsetSumWithDuplicates(possibles, tuple, 0, subsets, target_sum, 0);
}

void expansionHelper(Cage const *cage, unsigned idx, Mask m, IntList combo,
                     std::unordered_set<Cell const *> &used,
                     CageCombo &cage_combo) {
  std::size_t const cage_size = cage->size();
  // Loop over the cells in the cage in order until there are too few cells
  // left to make up the combination.
  for (unsigned ci = idx, ce = cage_size + 1 - m.count(); ci < ce; ++ci) {
    Cell const *cell = cage->cells[ci];
    for (unsigned i = 0, e = m.size(); i != e; i++) {
      if (!m[i] || !cell->candidates[i] || used.count(cell) != 0)
        continue;
      combo.push_back(i + 1);
      if (combo.size() == cage_size) {
        cage_combo.permutations.push_back(combo);
      } else {
        used.insert(cell);
        expansionHelper(cage, ci + 1, m & ~Mask(1 << i), combo, used,
                        cage_combo);
        used.erase(cell);
      }
      combo.pop_back();
    }
  }
}

void expandComboPermutations(Cage const *cage, CageCombo &cage_combo) {
  std::unordered_set<Cell const *> used;
  IntList combo;
  expansionHelper(cage, 0, cage_combo.combo, combo, used, cage_combo);
}
