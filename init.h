#ifndef COLUMBO_INIT_H
#define COLUMBO_INIT_H

#include "defs.h"
#include "utils.h"

#include <map>
#include <set>

#include <fstream>
#include <iostream>

static void skipSpace(const char **const ch, const char *end) {
  while (*ch != end && std::isspace(**ch)) {
    (*ch)++;
  }
}

static void getNextNonEmptyLine(std::ifstream &file, std::string &line) {
  while (std::getline(file, line) && line.empty())
    ;
}

static int intFromHex(const char c) {
  return (c >= 'a') ? (c - 'a' + 10)
                    : ((c >= 'A') ? (c - 'A' + 10) : (c - '0'));
}

int parseDec(const char **const ch, const char *end) {
  int num = 0;
  while (*ch != end && std::isdigit(**ch)) {
    int digit = **ch - '0';
    num = num * 10 + digit;
    (*ch)++;
  }
  return num;
}

int parseHex(const char **const ch, const char *end) {
  skipSpace(ch, end);

  if (*ch == end) {
    return -1;
  }

  int num = 0;
  const char *start = *ch;
  while (*ch != end && !std::isspace(**ch)) {
    const char c = **ch;
    if (!std::isxdigit(c)) {
      std::cout << "'" << c << "' is invalid...\n";
      return -1;
    }

    const int intval = intFromHex(c);

    num = (num << 4) | (intval & 0xF);
    (*ch)++;
  }

  if (num > 0x1FF) {
    std::cout << "'" << std::string(start, *ch) << "' is invalid...\n";
    return -1;
  }

  return num;
}

static bool parseCoordSet(Grid *grid, Cage *cage, const char **const ch,
                          const char *end) {
  std::vector<unsigned> rows;
  std::vector<unsigned> cols;
  while (std::isalpha(**ch) && *ch != end) {
    int upper = std::toupper(**ch);
    if (upper < 'A' || upper > 'J') {
      std::cout << "'" << *ch << "' is not a valid row...\n";
      return true;
    }

    if (upper == 'I') {
      std::cout << "Warning: try not to use 'I'. Use 'J' instead...";
    }

    // 'J' is special.
    if (upper == 'J') {
      upper = 'I';
    }

    int row = upper - 'A';
    rows.push_back(static_cast<unsigned>(row));
    ++(*ch);
  }

  if (!std::isdigit(**ch)) {
    std::cout << "Need a number...\n";
    return true;
  }

  // Now handle the numbers...
  while (*ch != end && std::isdigit(**ch)) {
    int col = **ch - '0';
    if (col < 0 || col > 8) {
      std::cout << "'" << *ch << "' is not a valid column...\n";
      return true;
    }
    cols.push_back(static_cast<unsigned>(col));
    ++(*ch);
  }

  if (rows.size() > 1 && cols.size() > 1) {
    std::cout
        << "Cannot specify more than one row and more than one column...\n";
    return true;
  }

  for (auto &row : rows) {
    for (auto &col : cols) {
      cage->addCells(grid, {{row, col}});
    }
  }

  if (cage->size() > 9) {
    std::cout << "Cage has too many cells (" << cage->size() << ")\n";
    return true;
  }

  skipSpace(ch, end);

  return false;
}

static bool initializeGridFromFile(std::ifstream &file, Grid *grid) {
  for (unsigned i = 0; i < 9; ++i) {
    std::string line;
    if (!std::getline(file, line) || line.empty()) {
      std::cout << "Not enough rows...\n";
      return true;
    }

    char *end = &(*line.end());
    const char *ch = &(*line.begin());

    // Parse a line of 9 numbers
    for (unsigned x = 0; x < 9; ++x) {
      const int num = parseHex(&ch, end);
      if (num < 0) {
        std::cout << "Could not find a number...\n";
        return true;
      }
      grid->getCell(i, x)->candidates =
          CandidateSet(static_cast<unsigned long>(num));
    }
  }

  std::string line;
  getNextNonEmptyLine(file, line);

  do {
    // Parse cage line
    char *end = &(*line.end());
    const char *ch = &(*line.begin());

    skipSpace(&ch, end);

    if (ch == end || !std::isdigit(*ch)) {
      std::cout << "Expecting a cage sum...\n";
      return true;
    }

    const int sum = parseDec(&ch, end);

    if (ch == end || !std::isspace(*ch)) {
      std::cout << "Expected a space between the sum and the cells...\n";
      return true;
    }

    skipSpace(&ch, end);

    auto cage = Cage(static_cast<unsigned>(sum));

    while (ch != end) {
      if (parseCoordSet(grid, &cage, &ch, end)) {
        return true;
      }
    }

    grid->cages.push_back(cage);

  } while (std::getline(file, line));

  return false;
}

static bool initializeCages(Grid *grid) {
  CageList &cages = grid->cages;

  unsigned total_sum = 0;
  for (auto &cage : cages) {
    total_sum += cage.sum;
    for (auto &cell : cage.cells) {
      cell->cage = &cage;
    }
  }

  std::array<bool, 81> seen_cells;
  std::fill(seen_cells.begin(), seen_cells.end(), false);

  bool invalid = false;
  for (auto &cage : cages) {
    for (auto &cell : cage.cells) {
      const unsigned idx = cell->coord.row * 9 + cell->coord.col;
      if (seen_cells[idx]) {
        invalid = true;
        std::cout << "Duplicated cell " << cell->coord << "\n";
      }
      seen_cells[idx] = true;
    }
  }

  for (unsigned row = 0; row < 9; ++row) {
    for (unsigned col = 0; col < 9; ++col) {
      auto *cell = grid->getCell(row, col);
      if (cell->cage) {
        continue;
      }
      invalid = true;
      std::cout << "No cage for " << cell->coord << "\n";
    }
  }

  if (total_sum != 405) {
    invalid = true;
    std::cout << "Error: cage total (" << total_sum << ") is not 405\n";
  }

  if (invalid) {
    return true;
  }

  // Create the cage graph: edges represent neighbours
  std::map<Cage *, std::set<Cage *>> cage_graph;
  for (unsigned row = 0; row < 9; ++row) {
    for (unsigned col = 0; col < 9; ++col) {
      auto *cell = grid->getCell(row, col);
      auto *right_cell = col != 8 ? grid->getCell(row, col + 1) : nullptr;
      if (right_cell && cell->cage != right_cell->cage) {
        cage_graph[cell->cage].insert(right_cell->cage);
        cage_graph[right_cell->cage].insert(cell->cage);
      }
      auto *down_cell = row != 8 ? grid->getCell(row + 1, col) : nullptr;
      if (down_cell && cell->cage != down_cell->cage) {
        cage_graph[cell->cage].insert(down_cell->cage);
        cage_graph[down_cell->cage].insert(cell->cage);
      }
    }
  }

  // Allocate colours to cages. Greedy graph colouring. Colour 0 is effectively
  // 'unassigned'.
  for (auto &cage : cages) {
    // Build up a bitmask of used colours
    Mask used_mask = 0;
    for (auto &neighbour : cage_graph[&cage]) {
      if (neighbour->colour) {
        used_mask |= (1 << (neighbour->colour - 1));
      }
    }
    // Now search for first unused colour
    unsigned i = 0;
    while (isOn(used_mask, i)) {
      ++i;
    }
    cage.colour = static_cast<int>(i + 1);
  }

  return false;
}

#endif // COLUMBO_INIT_H
