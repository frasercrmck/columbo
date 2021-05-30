#include "utils.h"

#include <algorithm>
#include <iostream>
#include <sstream>

// Given a list of lists of possible cage values:
//     [[1,2,3], [3,4,5]]
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
static void subsetSum(const std::vector<IntList> &possible_lists,
                      const std::size_t p_size, IntList &tuple,
                      unsigned tuple_sum, const Duplicates allow_duplicates,
                      std::vector<IntList> &subsets, const unsigned target_sum,
                      unsigned list_idx) {
  for (unsigned p = list_idx; p < p_size; ++p) {
    for (auto &poss : possible_lists[p]) {
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

      subsetSum(possible_lists, p_size, tuple, tuple_sum, allow_duplicates,
                subsets, target_sum, p + 1);

      tuple.pop_back();
      tuple_sum -= poss;
    }
  }
}

void generateSubsetSums(const unsigned target_sum,
                        const std::vector<IntList> &possibles,
                        const Duplicates allow_duplicates,
                        std::vector<IntList> &subsets) {
  IntList tuple;
  subsetSum(possibles, possibles.size(), tuple, 0, allow_duplicates, subsets,
            target_sum, 0);
}

/*
static const char *LIGHT_DOUBLE_DASH_HORIZONTAL = "\u254c"; // '╌'
*/
static const char *LIGHT_DOUBLE_DASH_VERTICAL = "\u254e"; // '╎'
static const char *DOUBLE_HORIZONTAL = "\u2550";          // '═'
static const char *DOUBLE_VERTICAL = "\u2551";            // '║'
static const char *DOUBLE_DOWN_AND_RIGHT = "\u2554";      // '╔'
static const char *DOUBLE_DOWN_AND_LEFT = "\u2557";       // '╗'
static const char *DOUBLE_UP_AND_RIGHT = "\u255a";        // '╚'
static const char *DOUBLE_UP_AND_LEFT = "\u255d";         // '╝'

static const char *DOUBLE_VERTICAL_AND_RIGHT = "\u2560"; // '╠'
static const char *DOUBLE_VERTICAL_AND_LEFT = "\u2563";  // '╣'

static const char *VERTICAL_DOUBLE_AND_RIGHT_SINGLE = "\u255f"; // '╟'
static const char *VERTICAL_DOUBLE_AND_LEFT_SINGLE = "\u2562";  // '╢'

static const char *DOUBLE_DOWN_AND_HORIZONTAL = "\u2566"; // '╦'
static const char *DOUBLE_UP_AND_HORIZONTAL = "\u2569";   // '╩'

static const char *DOUBLE_VERTICAL_AND_HORIZONTAL = "\u256c";        // '╬'
static const char *VERTICAL_SINGLE_AND_HORIZONTAL_DOUBLE = "\u256a"; // '╤'
static const char *VERTICAL_DOUBLE_AND_HORIZONTAL_SINGLE = "\u256b"; // '╫'

static const char *DOWN_SINGLE_AND_HORIZONTAL_DOUBLE = "\u2564"; // '╤'
static const char *UP_SINGLE_AND_HORIZONTAL_DOUBLE = "\u2567";   // '╧'

static const char *LIGHT_VERTICAL_AND_HORIZONTAL = "\u253c"; // '┼'

/*
static const char *LIGHT_UP = "\u2575";         // '╵'
static const char *LIGHT_LEFT = "\u2574";       // '╴'
*/
static const char *LIGHT_VERTICAL = "\u2502";   // '│'
static const char *LIGHT_HORIZONTAL = "\u2500"; // '─'

void printLine(const Grid *const grid, const unsigned row, const bool big_grid,
               const bool thick, const bool top, const bool bottom,
               const bool use_colour) {
  // Indent three spaces
  std::cout << "   ";

  // Print the initial box boundary
  const char *beg_vert_glyph = VERTICAL_DOUBLE_AND_RIGHT_SINGLE;
  if (thick) {
    if (top) {
      beg_vert_glyph = DOUBLE_DOWN_AND_RIGHT;
    } else if (bottom) {
      beg_vert_glyph = DOUBLE_UP_AND_RIGHT;
    } else {
      beg_vert_glyph = DOUBLE_VERTICAL_AND_RIGHT;
    }
  }
  std::cout << beg_vert_glyph;

  for (unsigned x = 0; x < 9; ++x) {
    // Print three horizontal lines
    const Cell &this_cell = grid->cells[row][x];
    const bool down_is_cage_buddy =
        !top && row != 8 && this_cell.cage == grid->cells[row + 1][x].cage;
    const char *horiz_glyph = LIGHT_HORIZONTAL;
    if (down_is_cage_buddy && use_colour) {
      std::cout << "\x1b[4" << this_cell.cage->colour << "m";
    }
    if (thick) {
      horiz_glyph = DOUBLE_HORIZONTAL;
    }
    // Small grid prints each cell 2 characters wide
    for (int z = 0; z < (big_grid ? 3 : 2); ++z) {
      std::cout << horiz_glyph;
    }

    if (down_is_cage_buddy && use_colour) {
      // Reset attributes
      std::cout << "\x1b[0m";
    }

    if (x == 2 || x == 5) {
      // If we're at a box intersection...
      const char *intersect_glyph = VERTICAL_DOUBLE_AND_HORIZONTAL_SINGLE;
      if (thick) {
        if (top) {
          intersect_glyph = DOUBLE_DOWN_AND_HORIZONTAL;
        } else if (bottom) {
          intersect_glyph = DOUBLE_UP_AND_HORIZONTAL;
        } else {
          intersect_glyph = DOUBLE_VERTICAL_AND_HORIZONTAL;
        }
      }
      std::cout << intersect_glyph;
    } else if (x != 8) {
      // If we're at a regular cell intersection...
      const char *intersect_glyph = LIGHT_VERTICAL_AND_HORIZONTAL;
      if (thick) {
        if (top) {
          intersect_glyph = DOWN_SINGLE_AND_HORIZONTAL_DOUBLE;
        } else if (bottom) {
          intersect_glyph = UP_SINGLE_AND_HORIZONTAL_DOUBLE;
        } else {
          intersect_glyph = VERTICAL_SINGLE_AND_HORIZONTAL_DOUBLE;
        }
      }
      std::cout << intersect_glyph;
    }
  }

  // Print the end of the line: a vertical bar
  const char *end_vert_glyph = VERTICAL_DOUBLE_AND_LEFT_SINGLE;

  if (thick) {
    if (top) {
      end_vert_glyph = DOUBLE_DOWN_AND_LEFT;
    } else if (bottom) {
      end_vert_glyph = DOUBLE_UP_AND_LEFT;
    } else {
      end_vert_glyph = DOUBLE_VERTICAL_AND_LEFT;
    }
  }

  std::cout << end_vert_glyph;
}

void printBigGridCell(const Cell &cell, unsigned row, bool use_colour) {
  if (use_colour) {
    // Coloured background
    std::cout << "\x1b[4" << cell.cage->colour << "m";
    // Black foreground
    std::cout << "\x1b[30m";
  }

  if (cell.isFixed()) {
    if (row != 1) {
      std::cout << "   ";
    } else {
      std::cout << " " << cell.isFixed() << " ";
    }
  } else {
    for (unsigned i = row * 3, e = i + 3; i < e; ++i) {
      if (cell.candidates[i]) {
        std::cout << i + 1;
      } else {
        std::cout << " ";
      }
    }
  }

  if (use_colour) {
    // Reset attributes
    std::cout << "\x1b[0m";
  }
}

static void printSmallGridCell(const Cell &cell, bool use_colour) {
  if (use_colour) {
    // Coloured background
    std::cout << "\x1b[4" << cell.cage->colour << "m";
    // Black foreground
    std::cout << "\x1b[30m";
  }

  Coord top_left = {9, 9};
  for (auto &other : *cell.cage) {
    if (other->coord.row < top_left.row) {
      top_left = other->coord;
    }
  }

  if (cell.coord == top_left) {
    if (cell.cage->sum < 10) {
      std::cout << " ";
    }
    std::cout << cell.cage->sum;
  } else {
    std::cout << "  ";
  }

  if (use_colour) {
    // Reset attributes
    std::cout << "\x1b[0m";
  }
}

void printRow(House &house, unsigned row, unsigned sub_row, bool big_grid,
              bool use_colour) {
  std::cout << " ";
  if (sub_row == 1) {
    std::cout << getID(row);
  } else {
    std::cout << " ";
  }
  std::cout << " ";
  std::cout << DOUBLE_VERTICAL;
  for (unsigned col = 0; col < 9; ++col) {
    if (big_grid) {
      printBigGridCell(*house[col], sub_row, use_colour);
    } else {
      printSmallGridCell(*house[col], use_colour);
    }
    const Cell &this_cell = *house[col];
    const char *vert_glyph = LIGHT_DOUBLE_DASH_VERTICAL;
    const bool right_is_cage_buddy =
        col != 8 && this_cell.cage == house[col + 1]->cage;
    const bool is_thick = col == 2 || col == 5 || col == 8;
    if (right_is_cage_buddy && use_colour) {
      std::cout << "\x1b[4" << this_cell.cage->colour << "m";
    }
    if (is_thick) {
      vert_glyph = DOUBLE_VERTICAL;
    } else {
      vert_glyph = LIGHT_VERTICAL;
    }
    std::cout << vert_glyph;

    if (right_is_cage_buddy && use_colour) {
      // Reset attributes
      std::cout << "\x1b[0m";
    }
  }
}

void printGrid(const Grid *const grid, bool use_colour, const char *phase) {

  if (phase != nullptr) {
    std::cout << "After " << phase << "...\n";
  }

  std::cout << "     ";
  for (unsigned i = 0; i < 9; ++i) {
    std::cout << i << "   ";
  }
  std::cout << "\n";

  printLine(grid, 0, /*big_grid*/ true, /*thick*/ true, /*top*/ true,
            /*bottom*/ false, use_colour);
  std::cout << "\n";

  for (unsigned row = 0; row < 9; ++row) {
    const bool in_small_grid = row >= 3 && row <= 5;

    unsigned small_row = (row - 3) * 3;

    printRow(*grid->rows[row], row, 0, /*big_grid*/ true, use_colour);
    if (in_small_grid) {
      printRow(*grid->rows[small_row], small_row, 0, /*big_grid*/ false,
               use_colour);
    }
    std::cout << "\n";

    printRow(*grid->rows[row], row, 1, /*big_grid*/ true, use_colour);
    if (in_small_grid) {
      printRow(*grid->rows[small_row + 1], small_row + 1, 0, /*big_grid*/ false,
               use_colour);
    }
    std::cout << "\n";

    printRow(*grid->rows[row], row, 2, /*big_grid*/ true, use_colour);
    if (in_small_grid) {
      printRow(*grid->rows[small_row + 2], small_row + 2, 0, /*big_grid*/ false,
               use_colour);
    }
    std::cout << "\n";

    if (row != 8) {
      printLine(grid, row, /*big_grid*/ true, row == 2 || row == 5,
                /*top*/ false,
                /*bottom*/ false, use_colour);
      if (row == 2) {
        // The line just before the small grid starts
        printLine(grid, 0, /*big_grid*/ false, true, /*top*/ true,
                  /*bottom*/ false, use_colour);
      } else if (in_small_grid) {
        printLine(grid, small_row + 2, /*big_grid*/ false, /*thick*/ true,
                  /*top*/ false,
                  /*bottom*/ row == 5, use_colour);
      }
      std::cout << "\n";
    }
  }

  printLine(grid, 8, /*big_grid*/ true, /*thick*/ true, /*top*/ false,
            /*bottom*/ true, use_colour);
  std::cout << "\n";
}

const char *getID(unsigned id) {
  switch (id) {
  default:
    return "X";
  case A:
    return "A";
  case B:
    return "B";
  case C:
    return "C";
  case D:
    return "D";
  case E:
    return "E";
  case F:
    return "F";
  case G:
    return "G";
  case H:
    return "H";
  case J:
    return "J";
  }
}

std::string getHousePrintNum(House &house) {
  switch (house.getKind()) {
  case HouseKind::Row:
    return getID(house.num);
  case HouseKind::Col:
  case HouseKind::Box: {
    std::stringstream ss;
    ss << house.num;
    return ss.str();
  }
  }
}

CellCountMaskArray collectCellCountMaskInfo(const House &house) {
  CellCountMaskArray cell_masks{};
  for (const auto &cell : house) {
    const auto &candidates = cell->candidates;
    for (std::size_t i = 0; i < 9; ++i) {
      if (candidates[i]) {
        cell_masks[i] |= (1 << house.getLinearID(cell));
      }
    }
  }
  return cell_masks;
}
