#include <glm/vec4.hpp>
#include <glm/detail/_swizzle.hpp>
#include <lights_manager.h>
#include <vulkan_tools.h>

namespace vks {

LightsManager::LightsManager() : lights_() {}

Light *LightsManager::CreateLight(const glm::vec3 &diffuse,
                                  const glm::vec3 &specular,
                                  const glm::vec3 &position, float radius) {
  Light light;
  light.pos_radius = glm::vec4(position, radius);
  light.diff_colour = diffuse;
  light.spec_colour = specular;

  lights_.push_back(light);
  return &lights_.back();
}

uint32_t LightsManager::GetNumLights() const {
  return SCAST_U32(lights_.size());
}

eastl::vector<Light>
LightsManager::TransformLights(const glm::mat4 &transform) {
  eastl::vector<Light> transformed_lights = lights_;
  uint32_t num_lights = GetNumLights();
  for (uint32_t i = 0U; i < num_lights; i++) {
    glm::vec4 pos(lights_[i].pos_radius.x, lights_[i].pos_radius.y,
                  lights_[i].pos_radius.z, 1.f);
    glm::vec4 new_pos = transform * pos;
    transformed_lights[i].pos_radius =
        glm::vec4(new_pos.x, new_pos.y, new_pos.z, lights_[i].pos_radius.w);
  }

  return transformed_lights;
}

void LightsManager::SetLightPosition(uint32_t light_idx,
		const glm::vec3 &new_position) {
  lights_[light_idx].pos_radius =
    glm::vec4(new_position, lights_[light_idx].pos_radius.w);
}

} // namespace vks
