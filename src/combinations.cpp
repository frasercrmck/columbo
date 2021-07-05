#include "combinations.h"
#include "debug.h"
#include "step.h"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <numeric>
#include <unordered_set>

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

static bool hasClash(IntList const &tuple, int tuple_index, int candidate,
                     std::vector<CellMask> const &clashes) {
  for (unsigned i = 0, e = tuple.size(); i != e; i++)
    if (tuple[i] == candidate && clashes[tuple_index][i])
      return true;
  return false;
}

static void subsetSumWithDuplicates(const std::vector<Mask> &possible_lists,
                                    const std::vector<CellMask> &clashes,
                                    IntList &tuple, unsigned tuple_sum,
                                    std::vector<CageCombo> &subsets,
                                    const unsigned target_sum,
                                    unsigned list_idx) {
  std::size_t const p_size = possible_lists.size();
  std::size_t const m_size = possible_lists[0].size();
  for (unsigned p = list_idx; p < p_size; ++p) {
    for (unsigned i = 0; i != m_size; ++i) {
      if (!possible_lists[p][i])
        continue;
      auto poss = i + 1;
      // Can't repeat a value inside a cage if those cells see each other.
      if (hasClash(tuple, p, poss, clashes))
        continue;
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
          // FIXME: This is dumb. We're finding the CageCombo which this
          // permutation fits in using a linear search. Surely we can keep
          // track of this as we go?
          Mask combo = 0, duplicates = 0;
          for (auto t : tuple) {
            if (combo[t - 1])
              duplicates.set(t - 1);
            combo.set(t - 1);
          }
          CageCombo c{combo};
          auto it = std::find_if(std::begin(subsets), std::end(subsets),
                                 [&](CageCombo const &cage_combo) {
                                   return cage_combo.combo == combo &&
                                          cage_combo.duplicates == duplicates;
                                 });
          if (it != std::end(subsets)) {
            it->permutations.push_back(tuple);
          } else {
            c.permutations = {tuple};
            subsets.push_back(c);
          }
          tuple.pop_back();
          break;
        }

        // Else, move on to the next candidate in the list.
        continue;
      }

      tuple_sum += poss;
      tuple.push_back(poss);

      subsetSumWithDuplicates(possible_lists, clashes, tuple, tuple_sum,
                              subsets, target_sum, p + 1);

      tuple.pop_back();
      tuple_sum -= poss;
    }
  }
}

std::vector<CageCombo>
generateSubsetSumsWithDuplicates(const unsigned target_sum,
                                      const std::vector<Mask> &possibles,
                                      const std::vector<CellMask> &clashes) {
  IntList tuple;
  std::vector<CageCombo> subsets;
  if (possibles.size() >= 32)
    throw invalid_grid_exception{"Too large a cage for the clash bitset"};
  subsetSumWithDuplicates(possibles, clashes, tuple, 0, subsets, target_sum, 0);
  return subsets;
}

static void expansionHelper(Cage const *cage, std::size_t idx, Mask m,
                            IntList combo,
                            std::unordered_set<Cell const *> &used,
                            CageCombo &cage_combo) {
  std::size_t const cage_size = cage->size();
  // Loop over the cells in the cage in order until there are too few cells
  // left to make up the combination.
  for (std::size_t ci = idx, ce = cage_size + 1 - m.count(); ci < ce; ++ci) {
    Cell const *cell = (*cage)[ci];
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
  expansionHelper(cage, 0u, cage_combo.combo, combo, used, cage_combo);
}

std::unordered_set<Mask> CageComboInfo::getUniqueCombinations() const {
  std::unordered_set<Mask> unique_combos;
  for (CageCombo const &cage_combo : *this)
    unique_combos.insert(cage_combo.combo);
  return unique_combos;
}

std::unordered_set<Mask> CageComboInfo::getUniqueCombinationsWithMask(
    CellMask const &cage_cell_mask) const {
  std::unordered_set<Mask> unique_combos;

  for (auto const &cage_combo : *cage->cage_combos) {
    for (auto const &perm : cage_combo.permutations) {
      Mask m = 0;
      for (unsigned i = 0, e = perm.size(); i != e; i++)
        m |= (cage_cell_mask[i] ? 1 : 0) << (perm[i] - 1);
      unique_combos.insert(m);
    }
  }

  return unique_combos;
}

std::unordered_set<Mask>
CageComboInfo::getUniqueCombinationsIn(House const &house) const {
  // Fast path
  if (cage->areAllCellsAlignedWith(house))
    return getUniqueCombinations();

  CellMask cell_mask = 0;

  for (unsigned i = 0, e = cage->size(); i != e; i++)
    cell_mask[i] = house.contains((*cage)[i]);

  return getUniqueCombinationsWithMask(cell_mask);
}

std::unordered_set<Mask>
CageComboInfo::getUniqueCombinationsWhichSee(Cell const *cell) const {
  // Fast path
  if (std::all_of(std::begin(cage->cells), std::end(cage->cells),
                  [cell](const Cell *c) { return c->canSee(cell); }))
    return getUniqueCombinations();

  CellMask cage_cell_mask = 0;

  for (unsigned i = 0, e = cage->size(); i != e; i++)
    cage_cell_mask[i] = (*cage)[i]->canSee(cell);

  return getUniqueCombinationsWithMask(cage_cell_mask);
}

std::unordered_set<Mask>
CageComboInfo::computeKillerPairs(unsigned max_size) const {
  CellMask cage_cell_mask;
  return computeKillerPairs(max_size, cage_cell_mask.set());
}

std::unordered_set<Mask>
CageComboInfo::computeKillerPairs(unsigned max_size,
                                  CellMask const &cell_mask) const {
  std::unordered_set<Mask> oneofs;

  if (cage->size() == 2 || cage->size() == 3) {
    bool size1 = size() < 2;
    bool size2 = size() < 3;
    for (unsigned i = 0; i < 9; i++) {
      if (!combos[0].combo[i])
        continue;
      for (unsigned j = size1 ? i : 0; j < (size1 ? i + 1 : 9); j++) {
        if (!size1 && !combos[1].combo[j])
          continue;
        Mask oneof(1 << i | 1 << j);
        for (unsigned k = size2 ? j : 0; k < (size2 ? j + 1 : 9); k++) {
          if (!size2 && !combos[2].combo[k])
            continue;
          Mask oneof(1 << i | 1 << j | 1 << k);
          if (cell_mask.all()) {
            if (std::all_of(begin(), end(), [oneof](CageCombo const &cc) {
                  return (cc.combo & oneof).any();
                }))
              oneofs.insert(oneof);
          } else {
            // Only return "oneof"s in the cells that we're interested in. This
            // may produce a more restricted subset.
            if (std::all_of(
                    begin(), end(), [oneof, &cell_mask](CageCombo const &cc) {
                      return std::all_of(
                          std::begin(cc.permutations),
                          std::end(cc.permutations),
                          [&oneof, &cell_mask](auto const &perm) {
                            Mask m = 0;
                            for (unsigned i = 0, e = perm.size(); i != e; i++)
                              m |= (cell_mask[i] ? 1 : 0) << (perm[i] - 1);
                            return (m & oneof).any();
                          });
                    })) {
              oneofs.insert(oneof);
            }
          }
        }
      }
    }
  }

  return oneofs;
}

using ::size_t;

static int signof(int val) { return (0 < val) - (val < 0); }

bool reduceBasedOnCageRelations(Cage &lhs, Cage &rhs, int sum, CellSet &changed,
                                bool debug, std::string const &debug_banner) {
  bool modified = false;
  int min_rhs = 0, max_rhs = 0;
  for (auto const *cell : rhs) {
    max_rhs += max_value(cell->candidates);
    min_rhs += min_value(cell->candidates);
  }
  int min_lhs = 0, max_lhs = 0;
  for (auto const *cell : lhs) {
    max_lhs += max_value(cell->candidates);
    min_lhs += min_value(cell->candidates);
  }

  int current_sum = max_lhs - max_rhs;

  if (sum == current_sum)
    return false;

  int const diff = sum - current_sum;

  std::tuple<Cage &, Cage &, int const &, int const &, int, int const &,
             int const &>
      to_check[2] = {{lhs, rhs, max_lhs, max_rhs, -1, min_lhs, min_rhs},
                     {rhs, lhs, max_rhs, max_lhs, 1, min_rhs, min_lhs}};
  bool printed = false;
  for (auto &[cage, other_cage, max, other_max, sign_val, min, other_min] :
       to_check) {
    if (cage.size() == 1 && signof(diff) == sign_val) {
      Cell *cell = cage[0];
      Mask stripped_mask = 0;
      for (size_t e = cell->candidates.size(),
                  j = static_cast<size_t>(max - std::abs(diff));
           j != e; j++)
        stripped_mask.set(j);
      if (auto intersection = ColumboStep::updateCell(
              cell, cell->candidates & ~stripped_mask, changed)) {
        if (debug) {
          if (!printed && !debug_banner.empty())
            dbgs() << debug_banner;
          printed = true;
          dbgs() << "\tRemoving " << printCandidateString(*intersection)
                 << " from " << cell->coord << " because "
                 << other_cage.printCellList() << " <= " << other_max << "\n";
        }
        modified = true;
      }
    }

    int min_tgt = other_min + (0 - sign_val) * sum;
    // FIXME: This is currently limited to when 'other_cage' has size 1, since
    // we incorrectly calculate the iminimum of 'other_cage' when the cells
    // each other. For example, the minimum of 16/2 isn't 2; it's 3 because we
    // can't have two 1s.
    if (min < min_tgt && other_cage.size() == 1) {
      for (auto *cell : cage) {
        // Calculate the maxes of the *other* cells in this cage, to infer the
        // minimum value *this* cell can hold.
        // TODO: We could optimize this further by constraining the max based
        // on cells which see each other.
        int other_maxes = 0;
        for (auto *other_cell : cage)
          if (cell != other_cell)
            other_maxes += max_value(other_cell->candidates);
        int min_diff = ((0 - sign_val) * sum) - other_maxes;
        // FIXME: What does this mean?
        if (min_diff <= 0)
          continue;
        // Calculate the strip mask for this cell.
        Mask stripped_mask = 0;
        for (size_t j = 0, e = static_cast<size_t>(min_diff); j != e; j++)
          stripped_mask.set(j);
        // Double-check we're not invalidating this cell
        if (stripped_mask.all())
          throw invalid_grid_exception{
              "Stripping would invalidate all candidates"};
        if (auto intersection = ColumboStep::updateCell(
                cell, cell->candidates & ~stripped_mask, changed)) {
          if (debug) {
            if (!printed && !debug_banner.empty())
              dbgs() << debug_banner;
            printed = true;
            dbgs() << "\tRemoving " << printCandidateString(*intersection)
                   << " from " << cell->coord << " because "
                   << other_cage.printCellList() << " >= " << other_min << "\n";
          }
          modified = true;
        }
      }
    }
  }
  return modified;
}

Mask CageCombo::comboMaskFromPermuation(IntList const &permutation) {
  CellMask m;
  return CageCombo::comboMaskFromPermuation(permutation, m.set());
}

Mask CageCombo::comboMaskFromPermuation(IntList const &permutation,
                                        CellMask cell_mask) {
  Mask combo_mask = 0;
  for (unsigned i = 0, e = permutation.size(); i != e; i++)
    if (cell_mask[i])
      combo_mask.set(permutation[i] - 1);
  return combo_mask;
}
