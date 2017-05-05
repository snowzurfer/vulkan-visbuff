#ifndef VKS_DEFERREDSCENE
#define VKS_DEFERREDSCENE

#include <camera.h>
#include <camera_controller.h>
#include <renderer.h>
#include <scene.h>

namespace vks {

const float kDefaultCameraSpeed = 40.f;
const float kDefaultCameraRotationSpeed = 30.f;

class VisbuffScene : public Scene {
public:
  VisbuffScene();

private:
  void DoInit();
  void DoRender(float delta_time);
  void DoUpdate(float delta_time);
  void DoShutdown();

  Renderer renderer_;
  szt::Camera cam_;
  szt::CameraController cam_controller_;

}; // class DeferredScene

} // namespace vks

#endif
