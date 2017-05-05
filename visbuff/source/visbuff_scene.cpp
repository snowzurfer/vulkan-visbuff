#include <EASTL/vector.h>
#include <assimp/postprocess.h>
#include <base_system.h>
#include <frustum.h>
#include <glm/gtc/matrix_transform.hpp>
#include <logger.hpp>
#include <meshes_heap_manager.h>
#include <model.h>
#include <model_manager.h>
#include <vertex_setup.h>
#include <visbuff_scene.h>
#include <vulkan_base.h>

namespace vks {

extern const int32_t kWindowWidth = 1920;
extern const int32_t kWindowHeight = 1080;
extern const char *kWindowName = "vksagres-visbuff";

VisbuffScene::VisbuffScene() : Scene(), renderer_(), cam_() {}

void VisbuffScene::DoInit() {
  input_manager()->SetCursorMode(window(), szt::MouseCursorMode::DISABLED);

  szt::Viewport viewport;
  viewport.x = 0U;
  viewport.y = 0U;
  viewport.width = kWindowWidth;
  viewport.height = kWindowHeight;
  szt::Frustum frustum(1.0f, 600.f, 40.f,
                       static_cast<float>(viewport.width) /
                           static_cast<float>(viewport.height));
  cam_.Init(viewport, frustum);

  cam_controller_.Init(window(), input_manager(), kDefaultCameraSpeed,
                       kDefaultCameraRotationSpeed);

  eastl::vector<VertexElement> vtx_layout;
  vtx_layout.push_back(VertexElement(VertexElementType::POSITION,
                                     SCAST_U32(sizeof(glm::vec3)),
                                     VK_FORMAT_R32G32B32_SFLOAT));
  vtx_layout.push_back(VertexElement(VertexElementType::NORMAL,
                                     SCAST_U32(sizeof(glm::vec3)),
                                     VK_FORMAT_R32G32B32_SFLOAT));
  vtx_layout.push_back(VertexElement(VertexElementType::UV,
                                     SCAST_U32(sizeof(glm::vec2)),
                                     VK_FORMAT_R32G32_SFLOAT));
  vtx_layout.push_back(VertexElement(VertexElementType::TANGENT,
                                     SCAST_U32(sizeof(glm::vec3)),
                                     VK_FORMAT_R32G32B32_SFLOAT));
  vtx_layout.push_back(VertexElement(VertexElementType::BITANGENT,
                                     SCAST_U32(sizeof(glm::vec3)),
                                     VK_FORMAT_R32G32B32_SFLOAT));

  VertexSetup vertex_setup(vtx_layout);

  renderer_.Init(&cam_, vertex_setup);

  Model *nanosuit = nullptr;
  model_manager()->LoadOtherModel(
      vulkan()->device(), STR(ASSETS_FOLDER) "models/crytek-sponza/sponza.dae",
      STR(ASSETS_FOLDER) "models/crytek-sponza/",
      // vulkan()->device(), STR(ASSETS_FOLDER) "models/rungholt/rungholt.obj",
      // STR(ASSETS_FOLDER) "models/rungholt/",
      aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals |
          aiProcess_Triangulate | aiProcess_JoinIdenticalVertices |
          aiProcess_FlipUVs,
      vertex_setup, &nanosuit);

  renderer_.RegisterModel(*nanosuit);
}

void VisbuffScene::DoRender(float delta_time) {
  renderer_.PreRender();
  renderer_.Render();
  renderer_.PostRender();
}

void VisbuffScene::DoUpdate(float delta_time) {
  cam_controller_.Update(&cam_, delta_time);

  // Reload shaders
  if (input_manager()->IsKeyPressed(GLFW_KEY_R)) {
    renderer_.ReloadAllShaders();
  }

  // Capture 20 frames worth of memory bandwidth data
  if (input_manager()->IsKeyPressed(GLFW_KEY_N)) {
    eastl::array<glm::vec3, 10U> positions = {
        glm::vec3(0.f, 4.f, 0.f),    glm::vec3(30.f, 4.f, 0.f),
        glm::vec3(-10.f, 10.f, 0.f), glm::vec3(2.f, 20.f, 2.f),
        glm::vec3(-40.f, 5.f, 0.f),  glm::vec3(0.f, 90.f, 0.f),
        glm::vec3(-60.f, 40.f, 0.f), glm::vec3(-10.f, 70.f, 0.f),
        glm::vec3(2.f, 20.f, 2.f),   glm::vec3(-40.f, 60.f, 0.f)};

    eastl::array<glm::vec3, 10U> directions = {
        glm::vec3(0.f, 1.f, -1.f), glm::vec3(1.f, 0.f, 0.f),
        glm::vec3(-1.f, 1.f, 0.f), glm::vec3(1.f, 1.f, 1.f),
        glm::vec3(1.f, 1.f, 0.f),  glm::vec3(-1.f, 1.f, 0.f),
        glm::vec3(1.f, 0.f, 0.f),  glm::vec3(-1.f, 0.f, 0.f),
        glm::vec3(1.f, -1.f, 1.f), glm::vec3(1.f, -0.5f, 0.f)};
    // eastl::array<glm::vec3, 10U> positions = {
    //    glm::vec3(0.f, 20.f, 0.f),    glm::vec3(30.f, 10.f, 0.f),
    //    glm::vec3(-90.f, 10.f, 0.f),  glm::vec3(2.f, 20.f, 2.f),
    //    glm::vec3(-40.f, 70.f, 50.f), glm::vec3(0.f, 90.f, 0.f),
    //    glm::vec3(120.f, 40.f, 0.f),  glm::vec3(-10.f, 80.f, 0.f),
    //    glm::vec3(2.f, 20.f, 2.f),    glm::vec3(-40.f, 60.f, 50.f)};

    // eastl::array<glm::vec3, 10U> directions = {
    //    glm::vec3(0.f, 0.f, -1.f),  glm::vec3(1.f, 0.f, 0.f),
    //    glm::vec3(-1.f, 1.f, 0.f),  glm::vec3(1.f, 1.f, 1.f),
    //    glm::vec3(1.f, -0.5f, 0.f), glm::vec3(-1.f, -0.5f, 0.f),
    //    glm::vec3(1.f, -0.5f, 1.f), glm::vec3(-1.f, 0.f, 1.f),
    //    glm::vec3(1.f, -1.f, 1.f),  glm::vec3(1.f, -0.5f, 0.f)};

    renderer_.CaptureBandwidthDataFromPositions(positions, directions);
  }

  // Capture 20 frames worth of memory bandwidth data
  if (input_manager()->IsKeyPressed(GLFW_KEY_C)) {
    renderer_.CaptureBandwidthDataAtPosition();
  }
}

void VisbuffScene::DoShutdown() { renderer_.Shutdown(); }

} // namespace vks
