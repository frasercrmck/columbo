#ifndef COLUMBO_XWINGS_H
#define COLUMBO_XWINGS_H

#include "debug.h"
#include "defs.h"
#include "nakeds.h"
#include "step.h"
#include <cassert>
#include <optional>

using NakedPair = Naked<2>;

struct XWing {
  Mask mask;
  std::pair<Coord, Coord> p1;
  std::pair<Coord, Coord> p2;
};

static std::optional<XWing> getXWing(const NakedPair &p1, const NakedPair &p2) {
  if (p1.mask != p2.mask) {
    return std::nullopt;
  }
  if (!p1.units[0].cell || !p1.units[1].cell || !p2.units[0].cell ||
      !p2.units[1].cell)
    return std::nullopt;
  if (p1.units[0].cell->coord.col != p2.units[0].cell->coord.col ||
      p1.units[1].cell->coord.col != p2.units[1].cell->coord.col) {
    return std::nullopt;
  }
  return XWing{
      p1.mask, std::make_pair(p1.units[0].cell->coord, p1.units[1].cell->coord),
      std::make_pair(p2.units[0].cell->coord, p2.units[1].cell->coord)};
}

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
        // Search the next rows for matching naked pairs
        for (std::size_t j = i + 1; j < e; ++j) {
          for (const auto &m : naked_pairs_per_row[j]) {
            if (auto xwing = getXWing(n, m)) {
              modified |= runOnXWing(grid, *xwing);
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

private:
  bool runOnXWing(Grid *const grid, const XWing &xwing) {
    bool modified = false;
    bool printed_xwing = false;
    const auto &c1 = grid->cols[xwing.p1.first.col];
    const auto &c2 = grid->cols[xwing.p1.second.col];
    for (auto *col : {c1.get(), c2.get()}) {
      for (auto *c : *col) {
        if (c->coord.row == xwing.p1.first.row ||
            c->coord.row == xwing.p2.first.row) {
          continue;
        }
        if (updateCell(c, ~xwing.mask)) {
          if (DEBUG) {
            if (!printed_xwing) {
              dbgs() << "X-Wing between " << xwing.p1.first << "/"
                     << xwing.p1.second << " & " << xwing.p2.first << "/"
                     << xwing.p2.second << " means that "
                     << printCandidateString(xwing.mask)
                     << " can be removed from:\n";
              printed_xwing = true;
            }
          }
          modified = true;
          changed.insert(c);
        }
      }
    }
    return modified;
  }
};

#endif // COLUMBO_XWINGS_H
