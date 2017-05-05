#ifndef VKS_SCENE
#define VKS_SCENE

#include <EASTL/array.h>
#define GLM_FORCE_CXX11
#include <glm/glm.hpp>
#include <cstdint>

namespace vks {
	
const uint32_t kNumLights_ = 300U;

class Scene {
public:
  Scene();
  virtual ~Scene();

  void Init();
  void Update(float delta_time);
  void Render(float delta_time);
  void Shutdown();

private:
  virtual void DoUpdate(float delta_time) = 0;
  virtual void DoRender(float delta_time) = 0;
  virtual void DoInit() = 0;
  virtual void DoShutdown() = 0;

	// Create lights that move up and down
	void CreateLights();
	// Update these lights' position and maintain them within
	// given boundaries
	void UpdateLights(float delta_time);

	// List of velocities per-light
	eastl::array<glm::vec3, kNumLights_> lights_vel_;

}; // class Scene

} // namespace vks

#endif
