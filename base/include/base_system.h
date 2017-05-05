#ifndef VKS_BASESYSTEM
#define VKS_BASESYSTEM

#include <cstdint>
#define GLFW_INCLUDE_VULKAN
#include <EASTL/unique_ptr.h>
#include <GLFW/glfw3.h>
#include <input_manager.h>
#include <lights_manager.h>
#include <material_manager.h>
#include <meshes_heap_manager.h>
#include <model_manager.h>
#include <scene.h>
#include <vulkan_base.h>
#include <vulkan_texture_manager.h>

namespace vks {

void Init();
// Run a given application; init its modules, run it, then perform its shutdown
void Run(Scene *scene);
void Shutdown();
// Signal engine to exit while running
void Exit();

GLFWwindow *window();
VulkanBase *vulkan();
MaterialManager *material_manager();
ModelManager *model_manager();
MeshesHeapManager *meshes_heap_manager();
VulkanTextureManager *texture_manager();
LightsManager *lights_manager();
szt::InputManager *input_manager();

} // namespace vks

#endif
