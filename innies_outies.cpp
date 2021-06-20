#include "innies_outies.h"
#include "combinations.h"
#include "debug.h"

#include <cassert>
#include <numeric>
#include <sstream>

bool EliminateOneCellInniesAndOutiesStep::reduceCombinations(
    const InnieOutieRegion &region, Cage &cage, unsigned sum,
    const char *cage_type, unsigned sum_lhs, unsigned sum_rhs, bool debug) {
  std::vector<IntList> subsets;

  if (cage.cage_combos) {
    for (auto &combo : *cage.cage_combos) {
      for (auto &v : combo.permutations) {
        subsets.push_back(v);
      }
    }
  } else {
    if (cage.doAllCellsSeeEachOther())
      throw invalid_grid_exception{"should have been pre-computed?"};
    std::vector<Mask> possibles;
    possibles.reserve(cage.cells.size());
    for (auto const *cell : cage.cells)
      possibles.push_back(cell->candidates);

    std::vector<std::bitset<32>> clashes;
    for (auto const *cell : cage.cells) {
      std::size_t i = 0;
      std::bitset<32> clash = 0;
      for (auto const *other_cell : cage.cells) {
        if (cell != other_cell && cell->canSee(other_cell))
          clash[i] = 1;
        i++;
      }
      clashes.push_back(clash);
    }


    generateSubsetSumsWithDuplicates(sum, possibles, clashes, subsets);
  }

  bool modified = false;
  bool have_printed_region = false;
  for (std::size_t i = 0, e = cage.size(); i < e; ++i) {
    Mask possibles_mask = 0u;
    Cell *cell = cage[i];

    for (auto &subset : subsets)
      possibles_mask |= (1 << (subset[i] - 1));

    if (updateCell(cell, possibles_mask)) {
      if (debug) {
        if (!have_printed_region) {
          have_printed_region = true;
          dbgs() << "Region " << region.getName() << " contains " << cage.size()
                 << " " << cage_type << " cells (";
          for (std::size_t c = 0, ce = cage.size(); c < ce; c++) {
            dbgs() << cage[c]->coord;
            if (c < ce - 1) {
              dbgs() << ",";
            }
          }
          dbgs() << ") which must add up to " << sum << " (" << sum_lhs << " - "
                 << sum_rhs << ")\n";
          dbgs() << "Given these combinations:\n";
        }
        dbgs() << "\tUpdating cell " << cell->coord << " to "
                  << printCandidateString(possibles_mask) << "\n";
      }
      modified |= true;
      changed.insert(cell);
      // TODO: Must this do region maintenance?
    }
  }

  return modified;
}

// TODO: InnieOutieRegion method?
void EliminateOneCellInniesAndOutiesStep::performRegionMaintenance(
    InnieOutieRegion &region) const {

  // Remove fixed cells from innie cages
  for (auto &innie : region.innies_outies) {
    unsigned sum = 0;
    auto i = std::remove_if(std::begin(*innie->inside_cage),
                            std::end(*innie->inside_cage), [&sum](Cell *c) {
                              sum += c->isFixed();
                              return c->isFixed() != 0;
                            });
    innie->sum -= sum;
    region.known_cage->sum += sum;
    region.known_cage->cells.insert(std::end(*region.known_cage), i,
                                    std::end(*innie->inside_cage));
    innie->inside_cage->cells.erase(i, std::end(*innie->inside_cage));
  }

  // Clean up innies/outies with no cells left
  // TODO: Revisit this. Are we removing opportunities here?
  region.innies_outies.erase(
      std::remove_if(std::begin(region.innies_outies),
                     std::end(region.innies_outies),
                     [](std::unique_ptr<InnieOutie> const &i) {
                       return i->inside_cage->size() == 0;
                     }),
      std::end(region.innies_outies));

  for (auto &outie : region.innies_outies) {
    unsigned sum = 0;
    auto i = std::remove_if(std::begin(*outie->outside_cage),
                            std::end(*outie->outside_cage), [&sum](Cell *c) {
                              sum += c->isFixed();
                              return c->isFixed() != 0;
                            });
    outie->sum -= sum;
    outie->outside_cage->cells.erase(i, std::end(*outie->outside_cage));
  }

  // Clean up outies with no cells left
  unsigned sum = 0;
  std::vector<Cell *> cells_to_add;
  auto i = std::remove_if(std::begin(region.innies_outies),
                          std::end(region.innies_outies),
                          [&sum, &cells_to_add](
                              std::unique_ptr<InnieOutie> const &o) {
                            if (!o->outside_cage->empty()) {
                              return false;
                            }
                            sum += o->sum;
                            cells_to_add.insert(
                                std::end(cells_to_add),
                                std::begin(o->inside_cage->cells),
                                std::end(o->inside_cage->cells));
                            return true;
                          });
  region.known_cage->sum += sum;
  region.known_cage->cells.insert(std::end(region.known_cage->cells),
                                 std::begin(cells_to_add),
                                 std::end(cells_to_add));
  region.innies_outies.erase(i, std::end(region.innies_outies));
}

bool EliminateOneCellInniesAndOutiesStep::runOnRegion(
    Grid *const grid, InnieOutieRegion &region,
    std::vector<InnieOutieRegion *> &to_remove, bool debug) {
  bool modified = false;

  performRegionMaintenance(region);

  const auto num_innie_outies = region.innies_outies.size();

  if (num_innie_outies == 0) {
    return false;
  }

  if (num_innie_outies == 1) {
    auto &first_innie_outie = region.innies_outies.front();

    if (first_innie_outie->inside_cage->size() == 1) {
      Cell *cell = (*first_innie_outie->inside_cage)[0];
      const auto innie_val = region.expected_sum - region.known_cage->sum;
      if (updateCell(cell, 1 << (innie_val - 1))) {
        if (debug) {
          dbgs() << "Setting one-cell innie " << cell->coord << " of region "
                 << region.getName() << " to " << innie_val << "; "
                 << region.expected_sum << " - " << region.known_cage->sum
                 << " = " << innie_val << "\n";
        }
        return true;
      }
    }

    if (first_innie_outie->outside_cage->size() == 1) {
      Cell *cell = (*first_innie_outie->outside_cage)[0];
      auto outie_val = (region.known_cage->sum + first_innie_outie->sum) -
                       region.expected_sum;
      if (updateCell(cell, 1 << (outie_val - 1))) {
        if (debug) {
          dbgs() << "Setting one-cell outie " << cell->coord << " of region "
                 << region.getName() << " to " << outie_val << "; "
                 << region.known_cage->sum + first_innie_outie->sum << " - "
                 << region.expected_sum << " = " << outie_val << "\n";
        }
        return true;
      }
    }

    if (first_innie_outie->inside_cage->size() > 1)
      if (runOnInnies(grid, region, region.innies, 2, 9, debug))
        return true;
  }

  if (num_innie_outies == 2) {
    for (auto const &[ins_idx, out_idx] :
         std::array<std::array<unsigned, 2>, 2>{{{0u, 1u}, {1u, 0u}}}) {
      // Check for innie/outie situations where two distinct cages have one
      // cell in and one cell out of the regeion. We can use this to infer
      // properties about the two cells in conjunction.
      if (region.innies_outies[ins_idx]->inside_cage->size() != 1 ||
          region.innies_outies[out_idx]->outside_cage->size() != 1)
        continue;
      // This gives us the sum of the outie cell minus the innie cell.
      int sum = static_cast<int>(region.known_cage->sum +
                                 region.innies_outies[out_idx]->sum) -
                static_cast<int>(region.expected_sum);
      auto *lhs = (*region.innies_outies[out_idx]->outside_cage)[0];
      auto *rhs = (*region.innies_outies[ins_idx]->inside_cage)[0];
      std::stringstream ss;
      ss << "Innies+Outies (Region " << region.getName() << "): " << lhs->coord
         << " - " << rhs->coord << " = " << sum << ":\n";
      // If the sum is zero, the cells must have equivalent solutions. We
      // can set their candidates to the intersection of the current sets.
      bool printed = false;
      if (sum == 0) {
        auto mask = lhs->candidates & rhs->candidates;
        for (auto *cell : {lhs, rhs}) {
          if (auto intersection = updateCell(cell, mask)) {
            if (debug) {
              if (!printed)
                dbgs() << ss.str();
              printed = true;
              dbgs() << "\tRemoving " << printCandidateString(*intersection)
                     << " from " << cell->coord << "\n";
            }
            modified = true;
          }
        }
      } else {
        // Else we can infer candidates using the difference between the two
        // cell values: if cell A - B = 1, then cell A can't have the value 1.
        if (sum < 0) {
          sum = -sum;
          std::swap(lhs, rhs);
        }
        std::size_t sumu = static_cast<std::size_t>(sum);
        Mask lhs_mask = lhs->candidates, rhs_mask = rhs->candidates;
        for (std::size_t i = 0, e = lhs->candidates.size(); i != e; i++) {
          if (i >= e - sumu)
            rhs_mask.reset(i);
          if (i < sumu || !lhs_mask[i] || !rhs_mask[i - sumu]) {
            lhs_mask.reset(i);
            if (i >= sumu)
              rhs_mask.reset(i - sumu);
          }
        }
        if (auto intersection = updateCell(lhs, lhs_mask)) {
          if (debug) {
            if (!printed)
              dbgs() << ss.str();
            printed = true;
            dbgs() << "\tRemoving " << printCandidateString(*intersection)
                   << " from " << lhs->coord << "\n";
          }
          modified = true;
        }
        if (auto intersection = updateCell(rhs, rhs_mask)) {
          if (debug) {
            if (!printed)
              dbgs() << ss.str();
            dbgs() << "\tRemoving " << printCandidateString(*intersection)
                   << " from " << rhs->coord << "\n";
          }
          modified = true;
        }
      }
    }
  }

  if (num_innie_outies == 2 || num_innie_outies == 3) {
    // Check for combinations of multi-cell innies/outies where we can infer
    // candidates based on the minimum/maximum of the combined cages. The same
    // rules apply as above.
    // Check all permutations of inside/outside cages, since cages may be both
    // in and out of the same region:
    //   YY|Y..|
    //    X|XX.|
    //   ZZ|Z..|
    // Check whether any/all of (YY-XXZ), (X-YZ), (Z-YXX) reach the expected
    // sum. If they don't, try and infer candidates based on the values. For
    // example, if (X-YZ) == -4 but currently equals -2 given X<=9 and YZ<=11
    // then we know that X is too high and can be trimmed down to X<=7.
    // TODO: There may be more we can do here, with the minimums
    for (unsigned i = 0; i < num_innie_outies; i++) {
      Cage inside, outside;
      if (region.innies_outies[i]->outside_cage->empty())
        continue;
      outside.sum = region.innies_outies[i]->sum;
      outside.cells.insert(std::end(outside.cells),
                           std::begin(*region.innies_outies[i]->outside_cage),
                           std::end(*region.innies_outies[i]->outside_cage));
      for (unsigned j = 0; j < num_innie_outies; j++) {
        if (i == j)
          continue;
        if (region.innies_outies[j]->inside_cage->empty())
          continue;
        inside.cells.insert(std::end(inside.cells),
                            std::begin(*region.innies_outies[j]->inside_cage),
                            std::end(*region.innies_outies[j]->inside_cage));
      }
      if (inside.empty())
        continue;
      int sum = static_cast<int>(region.known_cage->sum + outside.sum) -
                static_cast<int>(region.expected_sum);

      std::stringstream ss;
      ss << "Innies+Outies (Region " << region.getName() << "): ";
      outside.printCellList(ss);
      ss << " - ";
      inside.printCellList(ss);
      ss << " = " << sum << ":\n";
      if (reduceBasedOnCageRelations(outside, inside, sum, changed, debug, ss.str()))
        modified = true;
    }

    // TODO: Combine with above?
    for (unsigned i = 0; i < num_innie_outies; i++) {
      Cage inside, outside;
      if (region.innies_outies[i]->inside_cage->empty())
        continue;
      inside.cells.insert(std::end(inside.cells),
                          std::begin(*region.innies_outies[i]->inside_cage),
                          std::end(*region.innies_outies[i]->inside_cage));
      for (unsigned j = 0; j < num_innie_outies; j++) {
        if (i == j)
          continue;
        if (region.innies_outies[j]->outside_cage->empty())
          continue;
        outside.sum += region.innies_outies[j]->sum;
        outside.cells.insert(std::end(outside.cells),
                             std::begin(*region.innies_outies[j]->outside_cage),
                             std::end(*region.innies_outies[j]->outside_cage));
      }
      if (outside.empty())
        continue;
      int sum = static_cast<int>(region.known_cage->sum + outside.sum) -
                static_cast<int>(region.expected_sum);

      std::stringstream ss;
      ss << "Innies+Outies (Region " << region.getName() << "): ";
      outside.printCellList(ss);
      ss << " - ";
      inside.printCellList(ss);
      ss << " = " << sum << ":\n";
      if (reduceBasedOnCageRelations(outside, inside, sum, changed, debug, ss.str()))
        modified = true;
    }
  }

  if (num_innie_outies > 1 && std::count_if(std::begin(region.innies_outies),
                                            std::end(region.innies_outies),
                                            [](std::unique_ptr<InnieOutie> &i) {
                                              return i->inside_cage->size() > 1;
                                            }) == 0) {
    if (runOnInnies(grid, region, region.innies, 2, 4, debug))
      return true;
  }

  if (region.known_cage->size() == static_cast<std::size_t>(region.num_cells)) {
    to_remove.push_back(&region);
  }

  return modified;
}

bool EliminateOneCellInniesAndOutiesStep::runOnInnies(
    Grid *const grid, InnieOutieRegion &region,
    std::vector<std::unique_ptr<Cage>> &innies_list, int min_size, int max_size,
    bool debug) {
  auto pseudo_cage = std::make_unique<Cage>();
  for (auto &io : region.innies_outies)
    for (auto *c : *io->inside_cage)
      pseudo_cage->cells.push_back(c);

  if (pseudo_cage->empty())
    return false;

  if (region.known_cage->sum >= region.expected_sum)
    throw invalid_grid_exception{"invalid set of innies"};

  if (pseudo_cage->size() < min_size || pseudo_cage->size() > max_size)
    return false;

  pseudo_cage->is_pseudo = true;
  pseudo_cage->pseudo_name = region.getName() + " innies";
  pseudo_cage->sum = region.expected_sum - region.known_cage->sum;
  Cage *the_cage =
      getOrCreatePseudoCage(grid, region, innies_list, pseudo_cage);
  return reduceCombinations(region, *the_cage, the_cage->sum, "innie",
                            region.expected_sum, region.known_cage->sum, debug);
}

// Identify cases where a region's outies are split amongst different
// rows/cols/boxes and may form part of those houses' innies.
// In this example, column 5 containing Y and X/x is split between B1 and B2.
// Assuming that the two (lowercase) xs in B1 form B1's innies, we know their
// sum and may split the outie cage into [xx] and [XX]. This (hopefully) nets
// us some new subset sums with which to work.
//   |...|.Y.|...|
//   |..x|XX.|...|
//   |..x|XX.|...|
bool EliminateOneCellInniesAndOutiesStep::trySplitOutieCage(
    Grid *const grid, std::unique_ptr<Cage> &pseudo_cage,
    InnieOutieRegion &region, std::vector<std::unique_ptr<Cage>> &outies_list,
    std::vector<House const *> &houses, bool debug) {
  for (auto const *house : houses) {
    if (!house->region)
      continue;
    for (auto &innie_cage : house->region->innies) {
      if (std::all_of(std::begin(*innie_cage), std::end(*innie_cage),
                      [&pseudo_cage](Cell const *cell) {
                        return pseudo_cage->contains(cell);
                      })) {
        if (innie_cage->sum >= pseudo_cage->sum) {
          std::stringstream ss;
          ss << "Split cage candidate " << *innie_cage
             << " has a greater sum than its parent " << *pseudo_cage << "\n";
          throw invalid_grid_exception{ss.str()};
        }
        auto split_pseudo_cage = std::make_unique<Cage>();
        split_pseudo_cage->is_pseudo = true;
        split_pseudo_cage->pseudo_name = region.getName() + " split outies";
        split_pseudo_cage->sum = pseudo_cage->sum - innie_cage->sum;
        for (auto *c : *pseudo_cage) {
          if (!innie_cage->contains(c))
            split_pseudo_cage->cells.push_back(c);
        }
        Cage *the_split_cage = getOrCreatePseudoCage(
            grid, region, region.large_outies, split_pseudo_cage);
        if (!the_split_cage)
          throw invalid_grid_exception{"no cage?"};
        if (reduceCombinations(region, *the_split_cage, the_split_cage->sum,
                               "split outie",
                               region.known_cage->sum + the_split_cage->sum,
                               region.expected_sum, debug)) {
          return true;
        }
      }
    }
  }
  return false;
}

bool EliminateOneCellInniesAndOutiesStep::runOnOuties(
    Grid *const grid, InnieOutieRegion &region,
    std::vector<std::unique_ptr<Cage>> &outies_list, int min_size, int max_size,
    bool debug) {
  auto pseudo_cage = std::make_unique<Cage>();
  unsigned outie_cage_sum = 0;
  for (auto &io : region.innies_outies) {
    outie_cage_sum += io->sum;
    for (auto *c : *io->outside_cage)
      pseudo_cage->cells.push_back(c);
  }
  if (pseudo_cage->empty())
    return false;

  if (region.known_cage->sum + outie_cage_sum <= region.expected_sum)
    throw invalid_grid_exception{"invalid set of outies"};

  if (pseudo_cage->size() < min_size || pseudo_cage->size() > max_size)
    return false;

  pseudo_cage->is_pseudo = true;
  pseudo_cage->pseudo_name = region.getName() + " outies";
  pseudo_cage->sum =
      region.known_cage->sum + outie_cage_sum - region.expected_sum;

  std::set<House const *> rows, cols, boxes;
  for (auto *cell : *pseudo_cage) {
    rows.insert(cell->row);
    cols.insert(cell->col);
    boxes.insert(cell->box);
  }
  if (rows.size() == 2) {
    std::vector<House const *> boxes_vec(std::begin(rows), std::end(rows));
    if (trySplitOutieCage(grid, pseudo_cage, region, outies_list, boxes_vec,
                          debug))
      return true;
  }
  if (cols.size() == 2) {
    std::vector<House const *> boxes_vec(std::begin(cols), std::end(cols));
    if (trySplitOutieCage(grid, pseudo_cage, region, outies_list, boxes_vec,
                          debug))
      return true;
  }
  if (boxes.size() == 2) {
    std::vector<House const *> boxes_vec(std::begin(boxes), std::end(boxes));
    if (trySplitOutieCage(grid, pseudo_cage, region, outies_list, boxes_vec,
                          debug))
      return true;
  }

  Cage *the_cage =
      getOrCreatePseudoCage(grid, region, outies_list, pseudo_cage);
  return reduceCombinations(region, *the_cage, the_cage->sum, "outie",
                            region.known_cage->sum + outie_cage_sum,
                            region.expected_sum, debug);
}
