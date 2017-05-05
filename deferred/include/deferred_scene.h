#ifndef VKS_DEFERREDSCENE
#define VKS_DEFERREDSCENE

#include <camera.h>
#include <camera_controller.h>
#include <deferred_renderer.h>
#include <scene.h>

namespace vks {

const float kDefaultCameraSpeed = 80.f;
const float kDefaultCameraRotationSpeed = 50.f;

class DeferredScene : public Scene {
public:
  DeferredScene();

private:
  void DoInit();
  void DoRender(float delta_time);
  void DoUpdate(float delta_time);
  void DoShutdown();

  DeferredRenderer renderer_;
  szt::Camera cam_;
  szt::CameraController cam_controller_;

}; // class DeferredScene

} // namespace vks

#endif
