#ifndef SZT_VIEWPORT
#define SZT_VIEWPORT

#include <cstdint>

namespace szt {

struct Viewport {
  Viewport(uint32_t X = 0U, uint32_t Y = 0U, uint32_t W = 800U,
           uint32_t H = 600U)
      : x(X), y(Y), width(W), height(H) {}

  uint32_t x;
  uint32_t y;
  uint32_t width;
  uint32_t height;

}; // class Viewport

} // namespace szt

#endif
