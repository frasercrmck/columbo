#ifndef COLUMBO_PRINTABLE_H
#define COLUMBO_PRINTABLE_H

#include <functional>

class Printable {
public:
  std::function<void(std::ostream &os)> print;
  Printable(std::function<void(std::ostream &os)> print)
      : print(std::move(print)) {}
};

inline std::ostream &operator<<(std::ostream &os, const Printable &p) {
  p.print(os);
  return os;
}

#endif // COLUMBO_PRINTABLE_H
