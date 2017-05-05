#ifndef VKS_LIGHT
#define VKS_LIGHT

#define GLM_FORCE_CXX11
#include <glm/glm.hpp>

namespace vks {

struct Light {
  glm::vec4 pos_radius;
  glm::vec3 diff_colour;
  float padd;
  glm::vec3 spec_colour;
  float padd_2;
}; // struct Light

} // namespace vks

#endif
