#include <base_system.h>
#include <glm/gtc/random.hpp>
#include <glm/gtx/vec_swizzle.hpp>
#include <scene.h>

namespace vks {

Scene::Scene() {}

Scene::~Scene() {}

void Scene::Init() {
  CreateLights();
  DoInit();
  return;
}

void Scene::Update(float delta_time) {
  UpdateLights(delta_time);
  DoUpdate(delta_time);
}

void Scene::Shutdown() { DoShutdown(); }

void Scene::Render(float delta_time) { DoRender(delta_time); }

void Scene::CreateLights() {
  for (uint32_t i = 0U; i < kNumLights_; ++i) {
    glm::vec3 diff_colour = glm::linearRand(glm::vec3(1.f), glm::vec3(20.f));
    glm::vec3 pos = glm::linearRand(glm::vec3(-300.f), glm::vec3(300.f));

    lights_manager()->CreateLight(diff_colour, glm::vec3(10.f, 10.f, 10.f), pos,
                                  90.f);
    lights_vel_[i] =
        glm::linearRand(glm::vec3(0.f, -50.f, 0.f), glm::vec3(0.f, 50.f, 0.f));
  }
}

void Scene::UpdateLights(float delta_time) {
  for (uint32_t i = 0U; i < kNumLights_; ++i) {
    glm::vec3 new_pos = glm::xyz(lights_manager()->lights()[i].pos_radius) +
                        (lights_vel_[i] * delta_time);

    const glm::vec2 kLightYLimit(150.f, -10.f);
    // Contain lights within boundaries
    if (new_pos.y > kLightYLimit.x) {
      new_pos.y = kLightYLimit.x;
      lights_vel_[i].y *= -1.f;
    }
    if (new_pos.y < kLightYLimit.y) {
      new_pos.y = kLightYLimit.y;
      lights_vel_[i].y *= -1.f;
    }

    lights_manager()->SetLightPosition(i, new_pos);
  }
}

} // namespace vks
