#ifndef SOLVER_DEBUG_H
#define SOLVER_DEBUG_H

#include <string>
#include <sstream>
#include <iostream>

static bool DEBUG = true;

static std::string printCandidateString(unsigned long mask) {
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

std::ostream & dbgs() { return std::cout; }

#endif // SOLVER_DEBUG_H
