#ifndef SZT_CAMERACONTROLLER
#define SZT_CAMERACONTROLLER

struct GLFWwindow;

namespace szt {

class InputManager;
class Camera;

class CameraController {
public:
  CameraController();

  void Init(GLFWwindow *window, const InputManager *manager, float speed,
            float rotation_speed);
  void Update(Camera *cam, float delta_time);

private:
  const InputManager *manager_;
  GLFWwindow *window_;
  float speed_;
  float rotation_speed_;

}; // class CameraController

} // namespace szt

#endif
