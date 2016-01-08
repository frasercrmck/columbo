#ifndef COLUMBO_DEBUG_H
#define COLUMBO_DEBUG_H

#include <string>
#include <sstream>
#include <iostream>

#include "utils.h"
#include "defs.h"

static bool DEBUG = true;

static std::string printCandidateString(Mask mask) {
  std::stringstream ss;
  for (unsigned i = 0; i < 9; ++i) {
    if ((mask >> i) & 0x1) {
      ss << (i + 1) << '/';
    }
  }
  std::string s = ss.str();
  // Remove the trailing '/'
  return s.substr(0, s.size() - 1);
}

std::ostream &dbgs() { return std::cout; }

std::string printCellMask(House &house, const Mask mask) {
  std::stringstream ss;
  const int bit_count = bitCount(mask);
  int found = 0;
  for (unsigned i = 0; i < 9; ++i) {
    const bool is_on = (mask >> i) & 0x1;
    if (!is_on) {
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
