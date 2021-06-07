#include "combinations.h"
#include <algorithm>

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
static void subsetSum(const std::vector<Mask> &possible_lists, IntList &tuple,
                      unsigned tuple_sum, const Duplicates allow_duplicates,
                      std::vector<IntList> &subsets, const unsigned target_sum,
                      unsigned list_idx) {
  std::size_t const p_size = possible_lists.size();
  for (unsigned p = list_idx; p < p_size; ++p) {
    for (unsigned i = 0, e = possible_lists[p].size(); i != e; ++i) {
      if (!possible_lists[p][i])
        continue;
      auto poss = i + 1;
      // Optimization for small target sums: if the candidate is bigger than
      // the target itself then it can't be valid, neither can any candidate
      // after it (ordered).
      if (target_sum < static_cast<unsigned>(poss)) {
        break;
      }

      // Can't repeat a value inside a cage
      if (allow_duplicates == Duplicates::No &&
          std::find(tuple.begin(), tuple.end(), poss) != tuple.end()) {
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

      subsetSum(possible_lists, tuple, tuple_sum, allow_duplicates, subsets,
                target_sum, p + 1);

      tuple.pop_back();
      tuple_sum -= poss;
    }
  }
}

void generateSubsetSums(const unsigned target_sum,
                        const std::vector<Mask> &possibles,
                        const Duplicates allow_duplicates,
                        std::vector<IntList> &subsets) {
  IntList tuple;
  subsetSum(possibles, tuple, 0, allow_duplicates, subsets, target_sum, 0);
}

