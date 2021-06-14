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
  std::set<IntList> invalid_subsets;

  if (cage.cage_combos) {
    for (auto &combo : *cage.cage_combos) {
      for (auto &v : combo.permutations) {
        subsets.push_back(v);
      }
    }
  } else {
    std::vector<Mask> possibles;
    possibles.reserve(cage.cells.size());
    for (auto const *cell : cage.cells)
      possibles.push_back(cell->candidates);

    if (cage.doAllCellsSeeEachOther())
      throw invalid_grid_exception{"should have been pre-computed?"};

    generateSubsetSumsWithDuplicates(sum, possibles, subsets);
    // Strip out invalid subsets; those which repeat numbers for cells that
    // see each other
    for (const auto &subset : subsets) {
      bool invalid = false;
      for (std::size_t c1 = 0, ce = cage.size(); c1 < ce && !invalid; ++c1) {
        for (std::size_t c2 = c1 + 1; c2 < ce && !invalid; ++c2) {
          if (subset[c1] == subset[c2] && cage[c1]->canSee(cage[c2])) {
            invalid = true;
            invalid_subsets.insert(subset);
          }
        }
      }
    }
  }

  bool modified = false;
  bool have_printed_region = false;
  for (std::size_t i = 0, e = cage.size(); i < e; ++i) {
    Mask possibles_mask = 0u;
    Cell *cell = cage[i];

    for (auto &subset : subsets) {
      if (!invalid_subsets.count(subset)) {
        possibles_mask |= (1 << (subset[i] - 1));
      }
    }

    if (updateCell(cell, possibles_mask)) {
      if (debug) {
        if (!have_printed_region) {
          have_printed_region = true;
          dbgs() << "Region " << region.min << " - " << region.max
                 << " contains " << cage.size() << " " << cage_type
                 << " cells (";
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
    auto i = std::remove_if(std::begin(innie.inside_cage),
                            std::end(innie.inside_cage), [&sum](Cell *c) {
                              sum += c->isFixed();
                              return c->isFixed() != 0;
                            });
    innie.sum -= sum;
    region.known_cage.sum += sum;
    region.known_cage.cells.insert(std::end(region.known_cage), i,
                                   std::end(innie.inside_cage));
    innie.inside_cage.cells.erase(i, std::end(innie.inside_cage));
  }

  // Clean up innies/outies with no cells left
  // TODO: Revisit this. Are we removing opportunities here?
  region.innies_outies.erase(
      std::remove_if(std::begin(region.innies_outies),
                     std::end(region.innies_outies),
                     [](InnieOutie &i) { return i.inside_cage.size() == 0; }),
      std::end(region.innies_outies));

  for (auto &outie : region.innies_outies) {
    unsigned sum = 0;
    auto i = std::remove_if(std::begin(outie.outside_cage),
                            std::end(outie.outside_cage), [&sum](Cell *c) {
                              sum += c->isFixed();
                              return c->isFixed() != 0;
                            });
    outie.sum -= sum;
    outie.outside_cage.cells.erase(i, std::end(outie.outside_cage));
  }

  // Clean up outies with no cells left
  unsigned sum = 0;
  std::vector<Cell *> cells_to_add;
  auto i = std::remove_if(std::begin(region.innies_outies),
                          std::end(region.innies_outies),
                          [&sum, &cells_to_add](InnieOutie &o) {
                            if (!o.outside_cage.empty()) {
                              return false;
                            }
                            sum += o.sum;
                            cells_to_add.insert(std::end(cells_to_add),
                                                std::begin(o.inside_cage.cells),
                                                std::end(o.inside_cage.cells));
                            return true;
                          });
  region.known_cage.sum += sum;
  region.known_cage.cells.insert(std::end(region.known_cage.cells),
                                 std::begin(cells_to_add),
                                 std::end(cells_to_add));
  region.innies_outies.erase(i, std::end(region.innies_outies));
}

static int signof(int val) { return (0 < val) - (val < 0); }

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

    if (first_innie_outie.inside_cage.size() == 1) {
      Cell *cell = first_innie_outie.inside_cage[0];
      const auto innie_val = region.expected_sum - region.known_cage.sum;
      if (updateCell(cell, 1 << (innie_val - 1))) {
        if (debug) {
          dbgs() << "Setting one-cell innie " << cell->coord << " of region "
                 << region.getName() << " to " << innie_val << "; "
                 << region.expected_sum << " - " << region.known_cage.sum
                 << " = " << innie_val << "\n";
        }
        return true;
      }
    }

    if (first_innie_outie.outside_cage.size() == 1) {
      Cell *cell = first_innie_outie.outside_cage[0];
      auto outie_val =
          (region.known_cage.sum + first_innie_outie.sum) - region.expected_sum;
      if (updateCell(cell, 1 << (outie_val - 1))) {
        if (debug) {
          dbgs() << "Setting one-cell outie " << cell->coord << " of region "
                 << region.getName() << " to " << outie_val << "; "
                 << region.known_cage.sum + first_innie_outie.sum << " - "
                 << region.expected_sum << " = " << outie_val << "\n";
        }
        return true;
      }
    }

    if (first_innie_outie.inside_cage.size() > 1) {
      Cage pseudo_cage;
      pseudo_cage.is_pseudo = true;
      pseudo_cage.pseudo_name = region.getName() + " innies";
      pseudo_cage.sum = region.expected_sum - region.known_cage.sum;
      pseudo_cage.cells.insert(std::end(pseudo_cage.cells),
                               std::begin(first_innie_outie.inside_cage.cells),
                               std::end(first_innie_outie.inside_cage.cells));
      Cage *the_cage =
          getOrCreatePseudoCage(grid, region, region.innies, pseudo_cage);
      if (reduceCombinations(region, *the_cage, the_cage->sum, "innie",
                             region.expected_sum, region.known_cage.sum,
                             debug)) {
        return true;
      }
    }
  }

  if (num_innie_outies == 2) {
    for (auto const &[ins_idx, out_idx] :
         std::array<std::array<unsigned, 2>, 2>{{{0u, 1u}, {1u, 0u}}}) {
      // Check for innie/outie situations where two distinct cages have one
      // cell in and one cell out of the regeion. We can use this to infer
      // properties about the two cells in conjunction.
      if (region.innies_outies[ins_idx].inside_cage.size() != 1 ||
          region.innies_outies[out_idx].outside_cage.size() != 1)
        continue;
      // This gives us the sum of the outie cell minus the innie cell.
      int sum = static_cast<int>(region.known_cage.sum +
                                 region.innies_outies[out_idx].sum) -
                static_cast<int>(region.expected_sum);
      auto *lhs = region.innies_outies[out_idx].outside_cage[0];
      auto *rhs = region.innies_outies[ins_idx].inside_cage[0];
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
      if (region.innies_outies[i].outside_cage.empty())
        continue;
      outside.sum = region.innies_outies[i].sum;
      outside.cells.insert(std::end(outside.cells),
                           std::begin(region.innies_outies[i].outside_cage),
                           std::end(region.innies_outies[i].outside_cage));
      for (unsigned j = 0; j < num_innie_outies; j++) {
        if (i == j)
          continue;
        if (region.innies_outies[j].inside_cage.empty())
          continue;
        inside.cells.insert(std::end(inside.cells),
                            std::begin(region.innies_outies[j].inside_cage),
                            std::end(region.innies_outies[j].inside_cage));
      }
      if (inside.empty())
        continue;
      int sum = static_cast<int>(region.known_cage.sum + outside.sum) -
                static_cast<int>(region.expected_sum);
      int min_inside = 0, max_inside = 0;
      for (auto const *cell : inside) {
        max_inside += max_value(cell->candidates);
        min_inside += min_value(cell->candidates);
      }
      int min_outside = 0, max_outside = 0;
      for (auto const *cell : outside) {
        max_outside += max_value(cell->candidates);
        min_outside += min_value(cell->candidates);
      }

      int current_sum = max_outside - max_inside;
      if (sum == current_sum)
        continue;

      int const diff = sum - current_sum;

      bool printed = false;
      std::stringstream ss;
      ss << "Innies+Outies (Region " << region.getName() << "): ";
      outside.printCellList(ss);
      ss << " - ";
      inside.printCellList(ss);
      ss << " = " << sum << ":\n";
      std::tuple<Cage &, Cage &, int const &, int const &, int> to_check[2] = {
          {outside, inside, max_outside, max_inside, -1},
          {inside, outside, max_inside, max_outside, 1}};
      for (auto &[cage, other_cage, max, other_max, sign_val] : to_check) {
        if (cage.size() == 1 && signof(diff) == sign_val) {
          Mask stripped_mask = 0;
          for (std::size_t e = cage[0]->candidates.size(),
                           j = static_cast<std::size_t>(max - std::abs(diff));
               j != e; j++)
            stripped_mask.set(j);
          if (auto intersection =
                  updateCell(cage[0], cage[0]->candidates & ~stripped_mask)) {
            if (debug) {
              if (!printed) {
                printed = true;
                dbgs() << ss.str();
              }
              dbgs() << "\tRemoving " << printCandidateString(*intersection)
                     << " from " << cage[0]->coord << " because ";
              other_cage.printCellList(dbgs());
              dbgs() << " <= " << other_max << "\n";
            }
            modified = true;
          }
        }
      }
    }
  }

  if (num_innie_outies > 1 &&
      std::count_if(
          std::begin(region.innies_outies), std::end(region.innies_outies),
          [](InnieOutie &i) { return i.inside_cage.size() > 1; }) == 0) {
    Cage pseudo_cage;
    for (auto &i : region.innies_outies) {
      if (i.inside_cage.size() == 1) {
        pseudo_cage.cells.push_back(i.inside_cage[0]);
      }
    }

    if (pseudo_cage.size() <= 4) {
      pseudo_cage.is_pseudo = true;
      pseudo_cage.pseudo_name = region.getName() + " innies";
      pseudo_cage.sum = region.expected_sum - region.known_cage.sum;
      Cage *the_cage =
          getOrCreatePseudoCage(grid, region, region.innies, pseudo_cage);
      return reduceCombinations(region, *the_cage, the_cage->sum, "innie",
                                region.expected_sum, region.known_cage.sum,
                                debug);
    }
  }

  if (region.known_cage.size() == static_cast<std::size_t>(region.num_cells)) {
    to_remove.push_back(&region);
  }

  return modified;
}
