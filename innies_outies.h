#ifndef SOLVER_INNIES_OUTIES_H
#define SOLVER_INNIES_OUTIES_H

#include "defs.h"
#include "utils.h"
#include "debug.h"

#include <set>
#include <memory>
#include <cstdlib>

struct InnieOutieRegion {
  Coord min_house;
  Coord max_house;

  Cage innie_cage;
  Cage outie_cage;
  Cage known_cage;
  Cage unknown_cage;

  int num_cells;
  int expected_sum;

  InnieOutieRegion(Coord min, Coord max) : min_house(min), max_house(max) {
    unsigned rows = 1 + max_house.row - min_house.row;
    unsigned cols = 1 + max_house.col - min_house.col;

    num_cells = static_cast<int>(rows * cols);
    expected_sum = static_cast<int>(rows * cols) * 5;
  }

  void initialize(Grid *const grid) {
    std::set<Cage *> visited_cages;
    for (unsigned row = min_house.row; row <= max_house.row; ++row) {
      for (unsigned col = min_house.col; col <= max_house.col; ++col) {
        Cell *cell = getCell(grid, row, col);
        if (visited_cages.count(cell->cage)) {
          continue;
        }
        std::set<Cell *> cells_inside;
        std::set<Cell *> cells_outside;
        // Collect cells found inside and outside the cage
        for (auto &cage_cell : *cell->cage) {
          const Coord &coord = cage_cell->coord;
          if (coord.row >= min_house.row && coord.row <= max_house.row &&
              coord.col >= min_house.col && coord.col <= max_house.col) {
            cells_inside.insert(cage_cell);
          } else {
            cells_outside.insert(cage_cell);
          }
        }

        bool will_add_outie = cells_outside.size() == 1;

        if (cells_inside.size() == 1) {
          // We don't care about an outie in a 2-cell cage with one innie. The
          // outie will be solved automatically.
          will_add_outie &= cell->cage->cells.size() != 2;
          innie_cage.cells.push_back(*cells_inside.begin());
        } else if (cells_inside.size() == cell->cage->cells.size()) {
          // Add to the known total if all cells are inside
          for (auto &inside_cell : cells_inside) {
            known_cage.cells.push_back(inside_cell);
          }
          known_cage.sum += cell->cage->sum;
        } else {
          // Unknowns aren't very interesting if they're part of a cage
          // featuring an outie. They don't need to be known to solve an outie.
          // Once the outie is solved, we can mark them 'known'.
          if (!will_add_outie) {
            for (auto &inside_cell : cells_inside) {
              unknown_cage.cells.push_back(inside_cell);
            }
          }
        }
        if (will_add_outie) {
          outie_cage.cells.push_back(*cells_outside.begin());
        }
        visited_cages.insert(cell->cage);
      }
    }

    if (innie_cage.cells.empty()) {
      unknown_cage.sum = expected_sum - known_cage.sum;
    }
    innie_cage.sum = 0;
    if (unknown_cage.cells.empty()) {
      innie_cage.sum = expected_sum - known_cage.sum;
    }
    outie_cage.sum = 0;
  }
};

void initializeInnieAndOutieRegions(
    Grid *const grid,
    std::vector<std::unique_ptr<InnieOutieRegion>> &innies_and_outies) {
  // Do column-oriented regions
  const int max_width = 3;
  for (unsigned width = 1; width <= max_width; ++width) {
    for (unsigned col = 0; col <= 9 - width; ++col) {
      auto region = std::make_unique<InnieOutieRegion>(
          Coord{0, col}, Coord{8, col + width - 1});
      region->initialize(grid);
      if (region->known_cage.sum != region->expected_sum) {
        innies_and_outies.push_back(std::move(region));
      }
    }
  }

  // Do row-oriented regions
  for (unsigned width = 1; width <= max_width; ++width) {
    for (unsigned row = 0; row <= 9 - width; ++row) {
      auto region = std::make_unique<InnieOutieRegion>(
          Coord{row, 0}, Coord{row + width - 1, 8});
      region->initialize(grid);
      if (region->known_cage.sum != region->expected_sum) {
        innies_and_outies.push_back(std::move(region));
      }
    }
  }
}

static void updateKnownInsideCells(Cage &cage, Cage &known_cage) {
  int sum = 0;
  std::vector<Cell *> new_knowns;
  // Colect all fixed cells and tally up their totals
  auto iter = std::remove_if(cage.begin(), cage.end(), [&sum](Cell *cell) {
    if (cell->isFixed()) {
      sum += cell->isFixed();
      return true;
    }
    return false;
  });

  if (iter == cage.end()) {
    // Nothing to update
    return;
  }

  // Adjust sums
  if (cage.sum != 0) {
    cage.sum -= sum;
  }
  known_cage.sum += sum;

  // Transfer elements into known cage
  known_cage.cells.insert(known_cage.end(), iter, cage.end());
  cage.cells.erase(iter, cage.end());
}

static void addKnownsFromOutie(Cell *cell, Cage &outie_cage, Cage &known_cage) {
  const unsigned cell_val = cell->isFixed();
  if (outie_cage.sum != 0) {
    outie_cage.sum -= cell_val;
  }

  // Push all of its cage neighbours into the known cage
  for (auto &neighbour : *cell->cage) {
    if (neighbour == cell) {
      continue;
    }
    known_cage.cells.push_back(neighbour);
  }

  known_cage.sum += (static_cast<unsigned>(cell->cage->sum) - cell_val);
}

static void updateKnownOutsideCells(Cage &outie_cage, Cage &known_cage) {
  // Partition all fixed outies at the end of the cage
  auto iter = std::partition(outie_cage.begin(), outie_cage.end(),
                             [](Cell *cell) { return !cell->isFixed(); });

  if (iter == outie_cage.end()) {
    // Nothing to update
    return;
  }

  // For each fixed outie, add its cage neighbours to the known cells
  for (auto i = iter; i != outie_cage.end(); ++i) {
    addKnownsFromOutie(*i, outie_cage, known_cage);
  }

  // Remove outies
  outie_cage.cells.erase(iter, outie_cage.end());
}

static bool setOneCellInnie(InnieOutieRegion *region) {
  Cell *cell = region->innie_cage.cells[0];
  const int innie_val = region->expected_sum - region->known_cage.sum;
  cell->candidates = 1 << (innie_val - 1);

  if (DEBUG) {
    dbgs() << "Setting innie " << cell->coord << " of region ["
           << region->min_house << " - " << region->max_house << "]"
           << " to " << innie_val << "; " << region->expected_sum << " - "
           << region->known_cage.sum << " = " << innie_val << "\n";
  }

  // Remove it from the innie cage
  region->innie_cage.sum -= innie_val;
  region->innie_cage.cells.pop_back();
  // Push it into the known cage
  region->known_cage.sum += innie_val;
  region->known_cage.cells.push_back(cell);

  return true;
}

static bool setOneCellOutie(InnieOutieRegion *region) {
  Cell *cell = region->outie_cage.cells[0];

  const int cage_sum = cell->cage->sum;
  const int known_sum = region->known_cage.sum;

  const int outie_val = known_sum + cage_sum - region->expected_sum;
  cell->candidates = 1 << (outie_val - 1);

  if (DEBUG) {
    dbgs() << "Setting outie " << cell->coord << " of region ["
           << region->min_house << " - " << region->max_house << "]"
           << " to " << outie_val << "; (" << known_sum << " + " << cage_sum
           << " = " << (known_sum + cage_sum) << ") - " << region->expected_sum
           << " = " << outie_val << "\n";
  }

  addKnownsFromOutie(cell, region->outie_cage, region->known_cage);

  // Remove it from the outie cage
  region->outie_cage.cells.pop_back();

  return true;
}

static bool setLastUnknownCell(InnieOutieRegion *region) {
  if (region->unknown_cage.size() != 1) {
    return false;
  }

  if (!region->innie_cage.empty() || !region->outie_cage.empty()) {
    return false;
  }

  Cell *cell = region->unknown_cage.cells[0];
  const unsigned cell_val = region->expected_sum - region->known_cage.sum;
  if (DEBUG) {
    dbgs() << "Setting cell " << cell->coord << " of region ["
           << region->min_house << " - " << region->max_house << "]"
           << " to " << cell_val << "; " << region->expected_sum << " - "
           << region->known_cage.sum << " = " << cell_val << "\n";
  }

  cell->candidates = 1 << (cell_val - 1);

  // Remove it from the unknown cage
  region->unknown_cage.cells.pop_back();
  // Push it into the known cage
  region->known_cage.sum += cell_val;
  region->known_cage.cells.push_back(cell);

  return true;
}

bool eliminateOneCellInniesAndOuties(
    std::vector<std::unique_ptr<InnieOutieRegion>> &innies_and_outies) {
  bool modified = false;

  std::vector<std::unique_ptr<InnieOutieRegion> *> to_remove;

  for (auto &region : innies_and_outies) {
    // Update the block's innies, outies, and unknowns. They may have been
    // updated by another step.
    updateKnownInsideCells(region->innie_cage, region->known_cage);

    updateKnownInsideCells(region->unknown_cage, region->known_cage);

    updateKnownOutsideCells(region->outie_cage, region->known_cage);

    if (!region->unknown_cage.cells.empty()) {
      // Can't do anything with this yet, unless we have just one unknown cell
      // left, and no innies or outies.
      if (setLastUnknownCell(region.get())) {
        to_remove.push_back(&region);
      }

      continue;
    }

    const std::size_t num_innies = region->innie_cage.cells.size();
    const std::size_t num_outies = region->outie_cage.cells.size();

    if (num_innies > 1 || num_outies > 1) {
      continue;
    }

    if (num_innies == 1 && num_outies == 1) {
      continue;
    }

    if (num_innies == 1) {
      modified |= setOneCellInnie(region.get());
    } else if (num_outies == 1) {
      modified |= setOneCellOutie(region.get());
    }

    if (region->known_cage.cells.size() ==
        static_cast<std::size_t>(region->num_cells)) {
      to_remove.push_back(&region);
    }
  }

  // Remove uninteresting innie & outie regions
  while (!to_remove.empty()) {
    auto *ptr = to_remove.back();
    to_remove.pop_back();

    auto iter =
        std::remove(innies_and_outies.begin(), innies_and_outies.end(), *ptr);

    innies_and_outies.erase(iter, innies_and_outies.end());
  }

  return modified;
}

#endif // SOLVER_INNIES_OUTIES_H
