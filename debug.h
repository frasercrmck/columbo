#ifndef COLUMBO_DEBUG_H
#define COLUMBO_DEBUG_H

#include <iostream>
#include <sstream>
#include <string>

#include "defs.h"
#include "utils.h"

extern bool DEBUG;

static inline std::string printCandidateString(Mask mask) {
  std::stringstream ss;
  for (unsigned i = 0; i < 9; ++i) {
    if (mask[i]) {
      ss << (i + 1) << '/';
    }
  }
  std::string s = ss.str();
  // Remove the trailing '/'
  return s.substr(0, s.size() - 1);
}

static inline std::ostream &dbgs() { return std::cout; }

static inline std::string printCellMask(House &house, const Mask mask) {
  std::stringstream ss;
  std::size_t found = 0;
  const std::size_t bit_count = mask.count();
  for (unsigned i = 0; i < 9; ++i) {
    if (!mask[i]) {
      continue;
    }
    ++found;
    ss << house[i]->coord;
    if (found < bit_count) {
      ss << "/";
    }
  }
  return ss.str();
}

#endif // COLUMBO_DEBUG_H
