#include <eastl_streams.h>

std::ostream &operator<<(std::ostream &out, const eastl::string &str) {
  return out << str.c_str();
}
