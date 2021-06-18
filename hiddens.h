#ifndef COLUMBO_HIDDENS_H
#define COLUMBO_HIDDENS_H

#include "debug.h"
#include "defs.h"
#include "step.h"
#include "utils.h"

#include <algorithm>

struct EliminateHiddenSinglesStep : ColumboStep {
  bool runOnGrid(Grid *const grid, DebugOptions const &dbg_opts) override {
    changed.clear();
    bool modified = false;
    bool debug = dbg_opts.debug(getID());
    for (auto &row : grid->rows) {
      modified |= runOnHouse(*row, debug);
    }
    for (auto &col : grid->cols) {
      modified |= runOnHouse(*col, debug);
    }
    for (auto &box : grid->boxes) {
      modified |= runOnHouse(*box, debug);
    }
    return modified;
  }

  virtual void anchor() override;

  const char *getID() const override { return "hidden-singles"; }
  const char *getName() const override { return "Hidden Singles"; }

private:
  bool runOnHouse(House &house, bool debug);
};

template <int N> struct HiddenInfo {
  unsigned size = 0;
  bool invalid = false;
  Mask candidates = 0;
  std::array<CellCageUnit, N> units;

  void addDef(CellCageUnit const &unit) {
    if (size == N)
      invalid = true;
    else if (size < N)
      units[size++] = unit;
  }

  bool isFull() const { return size == N; }

  const char *getName() const {
    return N == 2 ? "Pair" : N == 3 ? "Triple" : N == 4 ? "Quad" : "Unknown";
  }

  typename std::array<CellCageUnit, N>::iterator end() {
    return units.begin() + size;
  }
  typename std::array<CellCageUnit, N>::iterator begin() {
    return units.begin();
  }

  typename std::array<CellCageUnit, N>::const_iterator end() const {
    return units.begin() + size;
  }
  typename std::array<CellCageUnit, N>::const_iterator begin() const {
    return units.begin();
  }

  bool operator==(const HiddenInfo &other) {
    return size == other.size && std::equal(std::begin(units), std::end(units),
                                            std::begin(other.units));
  }

  bool operator!=(const HiddenInfo &other) { return !operator==(other); }
};

template <int N> struct PairsOrTriplesOrQuadsStep : ColumboStep {
  bool runOnGrid(Grid *const grid, DebugOptions const &dbg_opts) override {
    changed.clear();
    bool modified = false;
    bool debug = dbg_opts.debug(getID());
    for (auto &row : grid->rows) {
      modified |= eliminateHiddens(*row, debug);
    }
    for (auto &col : grid->cols) {
      modified |= eliminateHiddens(*col, debug);
    }
    for (auto &box : grid->boxes) {
      modified |= eliminateHiddens(*box, debug);
    }
    return modified;
  }

protected:
  bool eliminateHiddens(House &house, bool debug);
};

template <int N>
bool PairsOrTriplesOrQuadsStep<N>::eliminateHiddens(House &house, bool debug) {
  bool modified = false;
  std::vector<HiddenInfo<N>> hidden_infos(9);

  for (unsigned i = 0, e = 9; i != e; i++)
    hidden_infos[i].candidates = 1 << i;

  for (auto *cell : house)
    for (unsigned i = 0, e = cell->candidates.size(); i != e; i++)
      if (cell->candidates[i])
        hidden_infos[i].addDef(CellCageUnit{cell});

  for (unsigned i = 0, e = 9; i != e; i++) {
    if (hidden_infos[i].size == 1 || hidden_infos[i].invalid)
      continue;
    for (unsigned j = i + 1, je = hidden_infos.size(); j != je; j++) {
      if (hidden_infos[j].size == 1 || hidden_infos[j].invalid)
        continue;
      Mask combined = hidden_infos[i].candidates | hidden_infos[j].candidates;
      if (combined.count() > N)
        continue;
      if (std::find_if(std::begin(hidden_infos) + j + 1, std::end(hidden_infos),
                       [combined](HiddenInfo<N> const &info) {
                         return info.candidates == combined;
                       }) == std::end(hidden_infos))
        continue;
      HiddenInfo<N> combined_hidden = hidden_infos[i];
      combined_hidden.candidates = combined;
      for (auto &unit : hidden_infos[j]) {
        if (std::none_of(std::begin(hidden_infos[i]), std::end(hidden_infos[i]),
                         [&unit](auto const &existing_unit) {
                           return existing_unit.overlapsWith(unit);
                         }))
          combined_hidden.addDef(unit);
      }
      if (!combined_hidden.invalid)
        hidden_infos.push_back(combined_hidden);
    }
  }

  for (auto const &hidden : hidden_infos) {
    if (hidden.size == 1 || hidden.invalid || hidden.candidates.count() != N)
      continue;
    for (auto *cell : house) {
      if (std::none_of(std::begin(hidden), std::end(hidden),
                       [cell](CellCageUnit const &unit) {
                         return unit.overlapsWith(cell);
                       }))
        continue;
      if (auto intersection = updateCell(cell, hidden.candidates)) {
        modified = true;
        if (debug) {
          dbgs() << "Hidden " << hidden.getName() << " "
                 << printCandidateString(hidden.candidates) << " removes "
                 << printCandidateString(*intersection) << " from "
                 << cell->coord << "\n";
        }
      }
    }
  }

  return modified;
}

struct EliminateHiddenPairsStep : public PairsOrTriplesOrQuadsStep<2> {
  virtual void anchor() override;

  const char *getID() const override { return "hidden-pairs"; }
  const char *getName() const override { return "Hidden Pairs"; }
};

struct EliminateHiddenTriplesStep : public PairsOrTriplesOrQuadsStep<3> {
  virtual void anchor() override;

  const char *getID() const override { return "hidden-triples"; }
  const char *getName() const override { return "Hidden Triples"; }
};

struct EliminateHiddenQuadsStep : public PairsOrTriplesOrQuadsStep<4> {
  virtual void anchor() override;

  const char *getID() const override { return "hidden-quads"; }
  const char *getName() const override { return "Hidden Quads"; }
};

#endif // COLUMBO_HIDDENS_H
