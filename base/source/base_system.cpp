#include <Timer.h>
#include <base_system.h>
#include <iostream>
#include <logger.hpp>
#include <scene.h>
#include <scene.h>
#include <utility>
#include <vulkan_base.h>
#include <vulkan_tools.h>

namespace vks {

static GLFWwindow *window_;
static Scene *scene_;
static bool done_ = false;
extern const int32_t kWindowWidth;
extern const int32_t kWindowHeight;
extern const char *kWindowName;

static Timer *timer() {
  static Timer timer_;
  return &timer_;
}

static void InitWindow() {
  glfwInit();

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

  if (kWindowHeight >= 1080) {
    window_ = glfwCreateWindow(kWindowWidth, kWindowHeight, kWindowName,
                               glfwGetPrimaryMonitor(), nullptr);
  } else {
    window_ = glfwCreateWindow(kWindowWidth, kWindowHeight, kWindowName,
                               nullptr, nullptr);
  }

  if (window_ == nullptr) {
    EXIT("GLFW window couldn't be created!");
  }
}

static void InitManagers() {
  texture_manager()->Init(vulkan()->device());
  input_manager()->Init(window());
}

static void ShutdownManagers() {
  texture_manager()->Shutdown(vulkan()->device());
  model_manager()->Shutdown(vulkan()->device());
  material_manager()->Shutdown(vulkan()->device());
  meshes_heap_manager()->Shutdown(vulkan()->device());
}

static void InitVulkan() {
  vulkan()->Init(window_, kWindowWidth, kWindowHeight);
}

static void InitApp(Scene *scene) { scene->Init(); }

static void MainLoop() {

  timer()->start();
  float delta_time = static_cast<float>(timer()->getElapsedTimeInSec());

  while (!done_) {
    if (glfwWindowShouldClose(window_)) {
      done_ = true;
      break;
    }

    glfwPollEvents();

    // switch (result) {
    // case VK_SUCCESS:
    // case VK_SUBOPTIMAL_KHR:
    // break;
    // case VK_ERROR_OUT_OF_DATE_KHR:
    //// Do something clever here
    // break;
    // default:
    // return false;
    //}

    scene_->Update(delta_time);

    scene_->Render(delta_time);

    input_manager()->EndFrame(window());

    delta_time = static_cast<float>(timer()->getElapsedTimeInSec());
    timer()->start();
  }
}

void Shutdown() {
  ShutdownManagers();

  // Do vulkan shutdown here
  vulkan()->Shutdown();

  glfwDestroyWindow(window_);
  glfwTerminate();
  LOG("Shutdown base system");
}

void ShutdownScene() { scene_->Shutdown(); }

void Init() {
  done_ = false;
  InitWindow();
  InitVulkan();
  InitManagers();
  LOG("Initialised system.");
}

void Run(Scene *scene) {
  scene_ = scene;
  InitApp(scene_);
  LOG("Initialised scene");
  MainLoop();
  LOG("Exiting main loop");
  ShutdownScene();
  LOG("Shutdown scene");
}

GLFWwindow *window() { return window_; }

VulkanBase *vulkan() {
  static VulkanBase vulkan_;
  return &vulkan_;
}

ModelManager *model_manager() {
  static ModelManager model_manager_;
  return &model_manager_;
}

VulkanTextureManager *texture_manager() {
  static VulkanTextureManager texture_manager_;
  return &texture_manager_;
}

MaterialManager *material_manager() {
  static MaterialManager material_manager_;
  return &material_manager_;
}

LightsManager *lights_manager() {
  static LightsManager lights_manager_;
  return &lights_manager_;
}

szt::InputManager *input_manager() {
  static szt::InputManager input_manager_;
  return &input_manager_;
}

MeshesHeapManager *meshes_heap_manager() {
  static MeshesHeapManager meshes_heap_manager;
  return &meshes_heap_manager;
}

void Exit() { done_ = true; }

} // namespace vks
