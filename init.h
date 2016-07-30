#ifndef COLUMBO_INIT_H
#define COLUMBO_INIT_H

#include "all_steps.h"
#include "defs.h"
#include "lexer.h"
#include "utils.h"

#include <map>
#include <set>

#include <fstream>
#include <iostream>

static bool parseCoordSet(Grid *grid, Cage *cage, const Tok &tok) {
  std::vector<unsigned> rows;
  std::vector<unsigned> cols;

  const char *end = tok.getStrEnd();
  const char *ptr = tok.getStrBegin();

  auto loc = tok.getStart();

  while (std::isalpha(*ptr) && ptr != end) {
    int upper = std::toupper(*ptr);
    if (upper < 'A' || upper > 'J') {
      std::cout << loc << ": '" << *ptr << "' is not a valid row...\n";
      return true;
    }

    if (upper == 'I') {
      std::cout << loc << ": warning: try not to use 'I'. Use 'J' instead...";
    }

    // 'J' is special.
    if (upper == 'J') {
      upper = 'I';
    }

    int row = upper - 'A';
    rows.push_back(static_cast<unsigned>(row));
    ++ptr;
  }

  if (!std::isdigit(*ptr)) {
    std::cout << loc << ": need a number...\n";
    return true;
  }

  // Now handle the numbers...
  while (ptr != end && std::isdigit(*ptr)) {
    int col = *ptr - '0';
    if (col < 0 || col > 8) {
      std::cout << loc << ": '" << *ptr << "' is not a valid column...\n";
      return true;
    }
    cols.push_back(static_cast<unsigned>(col));
    ++ptr;
  }

  if (rows.size() > 1 && cols.size() > 1) {
    std::cout
        << loc
        << ": cannot specify more than one row and more than one column...\n";
    return true;
  }

  for (auto &row : rows) {
    for (auto &col : cols) {
      cage->addCells(grid, {{row, col}});
    }
  }

  if (cage->size() > 9) {
    std::cout << loc << ": cage has too many cells (" << cage->size() << ")\n";
    return true;
  }

  return false;
}

static bool initializeGridFromFile(std::ifstream &file, Grid *grid) {
  std::string content((std::istreambuf_iterator<char>(file)),
                      (std::istreambuf_iterator<char>()));
  Tok tok;
  unsigned row = 0;
  unsigned col = 0;

  Lexer lex(content);

  while (true) {
    tok = lex.lex();
    if (!tok) {
      std::cout << "Error parsing file...\n";
      return true;
    }

    if (tok.getKind() == TKind::EndOfFile) {
      break;
    }

    if (!tok.isNumber()) {
      std::cout << tok.getStart() << ": unexpected non-number '" << tok.str()
                << "' when parsing cell candidate sets\n";
      return true;
    }

    if (tok.getNum() > 0x1FF) {
      std::cout << tok.getStart() << ": number '" << tok.str()
                << "' is too large. Must be <= 0x1FF.\n";
      return true;
    }

    grid->getCell(row, col)->candidates =
        CandidateSet(static_cast<unsigned long>(tok.getNum()));

    ++col;
    if (col == 9) {
      ++row;
      col = 0;
    }

    if (row == 9) {
      break;
    }
  }

  if (tok.getKind() == TKind::EndOfFile) {
    std::cout << "No cage values\n";
    return true;
  }

  while (true) {
    tok = lex.lex();
    if (!tok) {
      std::cout << "Error parsing file...\n";
      return true;
    }

    if (tok.getKind() == TKind::EndOfFile) {
      break;
    }

    if (!tok.isNumber()) {
      std::cout << tok.getStart() << ": unexpected non-number '" << tok.str()
                << "' when parsing cage lines\n";
      return true;
    }

    const int cage_sum = tok.getNum();
    auto cage = Cage(static_cast<unsigned>(cage_sum));

    // Consume the sum
    tok = lex.lex();

    while (tok && tok.getKind() == TKind::String) {
      if (parseCoordSet(grid, &cage, tok)) {
        std::cout << tok.getStart() << ": '" << tok.str()
                  << "' - failed to parse coord set\n";
        return true;
      }
      tok = lex.lex(SkipNewLine::False);
    }

    if (!tok) {
      std::cout << "Failed to parse coord set\n";
      return true;
    }

    grid->cages.push_back(cage);

    if (tok.getKind() == TKind::EndOfFile) {
      break;
    }
  }

  return false;
}

static bool initializeCages(Grid *grid) {

  // Clean up grid based on initial fixed cells
  CellSet fixed_cells;
  auto OLD_DEBUG = DEBUG;
  DEBUG = false;
  for (auto &r : grid->cells) {
    for (auto &c : r) {
      if (c.isFixed()) {
        fixed_cells.insert(&c);
      }
    }
  }
  auto cleanup_step = std::make_unique<PropagateFixedCells>();
  cleanup_step->setWorkList(fixed_cells);
  cleanup_step->runOnGrid(grid);
  DEBUG = OLD_DEBUG;

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
