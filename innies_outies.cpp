#include "innies_outies.h"
#include "debug.h"

#include <numeric>

bool EliminateOneCellInniesAndOutiesStep::reduceCombinations(
    const InnieOutieRegion &region, const Cage &cage, unsigned sum,
    const char *cage_type) {
  std::vector<IntList> possibles;
  possibles.resize(cage.size());

  unsigned idx = 0;
  for (const auto *cell : cage) {
    for (unsigned x = 0; x < 9; ++x) {
      if (cell->candidates[x]) {
        possibles[idx].push_back(x + 1);
      }
    }
    ++idx;
  }

  std::vector<IntList> subsets;
  generateSubsetSums(sum, possibles, Duplicates::Yes, subsets);

  // Strip out invalid subsets; those which repeat numbers for cells that see
  // each other
  std::set<IntList> invalid_subsets;
  for (const auto &subset : subsets) {
    bool invalid = false;
    for (std::size_t c1 = 0, ce = cage.size(); c1 < ce && !invalid; ++c1) {
      for (std::size_t c2 = c1 + 1; c2 < ce && !invalid; ++c2) {
        if (subset[c1] == subset[c2] &&
            cage.cells[c1]->canSee(cage.cells[c2])) {
          invalid = true;
          invalid_subsets.insert(subset);
        }
      }
    }
  }

  bool modified = false;
  bool have_printed_region = false;
  for (std::size_t i = 0, e = cage.size(); i < e; ++i) {
    Mask possibles_mask = 0u;
    Cell *cell = cage.cells[i];

    for (auto &subset : subsets) {
      if (!invalid_subsets.count(subset)) {
        possibles_mask |= (1 << (subset[i] - 1));
      }
    }

    if (updateCell(cell, possibles_mask)) {
      if (DEBUG) {
        if (!have_printed_region) {
          have_printed_region = true;
          std::cout << "Region " << region.min << " - " << region.max
                    << " contains " << cage.size() << " " << cage_type
                    << " cells (";
          for (std::size_t c = 0, ce = cage.size(); c < ce; c++) {
            std::cout << cage.cells[c]->coord;
            if (c < ce - 1) {
              std::cout << ",";
            }
          }
          std::cout << ") which must add up to " << sum << " ("
                    << region.expected_sum << " - " << cage.sum << ")\n";
          std::cout << "Given these combinations:\n";
        }
        std::cout << "\tUpdating cell " << cell->coord << " to "
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
  for (auto &innie : region.innies) {
    unsigned sum = 0;
    auto i = std::remove_if(innie.cell_cage.begin(), innie.cell_cage.end(),
                            [&sum](Cell *c) {
                              sum += c->isFixed();
                              return c->isFixed() != 0;
                            });
    region.known_cage.sum += sum;
    region.known_cage.cells.insert(region.known_cage.end(), i,
                                   innie.cell_cage.end());
    innie.cell_cage.cells.erase(i, innie.cell_cage.end());
  }

  // Clean up innies with no cells left
  region.innies.erase(
      std::remove_if(region.innies.begin(), region.innies.end(),
                     [](InnieOutie &i) { return i.cell_cage.size() == 0; }),
      region.innies.end());

  for (auto &outie : region.outies) {
    unsigned sum = 0;
    auto i = std::remove_if(outie.cell_cage.begin(), outie.cell_cage.end(),
                            [&sum](Cell *c) {
                              sum += c->isFixed();
                              return c->isFixed() != 0;
                            });
    outie.sum -= sum;
    outie.cell_cage.cells.erase(i, outie.cell_cage.end());
  }

  // Clean up outies with no cells left
  unsigned sum = 0;
  std::vector<Cell *> cells_to_add;
  auto i = std::remove_if(region.outies.begin(), region.outies.end(),
                          [&sum, &cells_to_add](InnieOutie &o) {
                            if (o.cell_cage.size() != 0) {
                              return false;
                            }
                            sum += o.sum;
                            cells_to_add.insert(cells_to_add.end(),
                                                o.unknown_cage.cells.begin(),
                                                o.unknown_cage.cells.end());
                            return true;
                          });
  region.known_cage.sum += sum;
  region.known_cage.cells.insert(region.known_cage.cells.end(),
                                 cells_to_add.begin(), cells_to_add.end());
  region.outies.erase(i, region.outies.end());
}

bool EliminateOneCellInniesAndOutiesStep::runOnRegion(
    InnieOutieRegion &region, std::vector<InnieOutieRegion *> &to_remove) {
  bool modified = false;

  performRegionMaintenance(region);

  const auto num_innies = region.innies.size();
  const auto num_outies = region.outies.size();

  if (num_innies != 0 && num_outies != 0) {
    return false;
  }

  if (num_innies == 1 && region.innies[0].cell_cage.size() == 1) {
    Cell *cell = region.innies[0].cell_cage.cells[0];
    const auto innie_val = region.expected_sum - region.known_cage.sum;
    if (updateCell(cell, 1 << (innie_val - 1))) {
      if (DEBUG) {
        dbgs() << "Setting innie " << cell->coord << " of region "
               << region.getName() << " to " << innie_val << "; "
               << region.expected_sum << " - " << region.known_cage.sum << " = "
               << innie_val << "\n";
      }
      return true;
    }
  }

  if (num_outies == 1 && region.outies[0].cell_cage.size() == 1) {
    InnieOutie &outie = region.outies[0];
    Cell *cell = outie.cell_cage.cells[0];
    auto outie_val = (region.known_cage.sum + outie.sum) - region.expected_sum;
    if (updateCell(cell, 1 << (outie_val - 1))) {
      if (DEBUG) {
        dbgs() << "Setting outie " << cell->coord << " of region "
               << region.getName() << " to " << outie_val << "; "
               << region.expected_sum << " - " << region.known_cage.sum << " = "
               << outie_val << "\n";
      }
      return true;
    }
  }

  if (num_innies == 1 && region.innies[0].cell_cage.size() > 1) {
    const auto sum = region.expected_sum - region.known_cage.sum;
    return reduceCombinations(region, region.innies[0].cell_cage, sum, "innie");
  }

  if (num_innies > 1 && std::count_if(region.innies.begin(),
                                      region.innies.end(), [](InnieOutie &i) {
                                        return i.cell_cage.size() > 1;
                                      }) == 0) {
    Cage pseudo_cage;
    for (auto &i : region.innies) {
      if (i.cell_cage.size() == 1) {
        pseudo_cage.cells.push_back(i.cell_cage.cells[0]);
      }
    }
    if (pseudo_cage.size() <= 4) {
      unsigned sum = region.expected_sum - region.known_cage.sum;
      return reduceCombinations(region, pseudo_cage, sum, "innie");
    }
  }

  if (region.known_cage.size() == static_cast<std::size_t>(region.num_cells)) {
    to_remove.push_back(&region);
  }

  return modified;
}
