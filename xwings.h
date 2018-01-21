#ifndef COLUMBO_XWINGS_H
#define COLUMBO_XWINGS_H

#include "debug.h"
#include "defs.h"
#include "nakeds.h"
#include "step.h"
#include <cassert>

using NakedPair = Naked<2>;
struct XWingsStep : ColumboStep {
  bool runOnGrid(Grid *const grid) override {
    changed.clear();
    bool modified = false;

    std::array<std::vector<NakedPair>, 9> naked_pairs_per_row;
    { // Collect naked pairs per row
      unsigned i = 0;
      for (auto &row : grid->rows) {
        naked_pairs_per_row[i++] = getNakeds<2>(*row);
      }
    }

    for (std::size_t i = 0, e = grid->rows.size(); i < e; ++i) {
      for (const auto &n : naked_pairs_per_row[i]) {
        auto coords =
            std::make_pair(n.cells[0]->coord.col, n.cells[1]->coord.col);

        // Search the next rows for matching naked pairs
        for (std::size_t j = i + 1; j < e; ++j) {
          for (const auto &m : naked_pairs_per_row[j]) {
            if (m.mask != n.mask) {
              continue;
            }
            auto c =
                std::make_pair(m.cells[0]->coord.col, m.cells[1]->coord.col);
            if (c == coords) {
              std::cout << n.cells[0]->coord << " & " << n.cells[1]->coord
                        << "\n";
              std::cout << m.cells[0]->coord << " & " << m.cells[1]->coord
                        << "\n";
              assert(false && "found a x-wing");
            }
          }
        }
      }
    }

    return modified;
  }

  virtual void anchor() override;

  const char *getID() const override { return "x-wings"; }
  const char *getName() const override { return "X-Wings"; }
};

#endif // COLUMBO_XWINGS_H
