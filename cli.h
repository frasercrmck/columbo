#include <string_view>

static inline bool isOpt(std::string_view arg, std::string_view flag,
                         std::string_view name) {
  return arg == flag || arg == name;
}
