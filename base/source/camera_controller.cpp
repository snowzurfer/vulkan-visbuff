#include <GLFW/glfw3.h>
#include <camera.h>
#include <camera_controller.h>
#include <input_manager.h>

namespace szt {

CameraController::CameraController()
    : manager_(nullptr), window_(nullptr), speed_(0.f), rotation_speed_(0.f) {}

void CameraController::Init(GLFWwindow *window, const InputManager *manager,
                            float speed, float rotation_speed) {
  manager_ = manager;
  window_ = window;
  speed_ = speed;
  rotation_speed_ = rotation_speed;
}

void CameraController::Update(Camera *cam, float delta_time) {
  if (manager_->IsKeyDown(GLFW_KEY_W)) {
    cam->set_position(cam->position() +
                      (cam->GetForwardVector() * speed_ * delta_time));
  }
  if (manager_->IsKeyDown(GLFW_KEY_S)) {
    cam->set_position(cam->position() -
                      (cam->GetForwardVector() * speed_ * delta_time));
  }
  if (manager_->IsKeyDown(GLFW_KEY_A)) {
    cam->set_position(cam->position() -
                      (cam->GetRightVector() * speed_ * delta_time));
  }
  if (manager_->IsKeyDown(GLFW_KEY_D)) {
    cam->set_position(cam->position() +
                      (cam->GetRightVector() * speed_ * delta_time));
  }
  if (manager_->IsKeyDown(GLFW_KEY_Q)) {
    cam->set_position(cam->position() +
                      (cam->GetUpVector() * speed_ * delta_time));
  }
  if (manager_->IsKeyDown(GLFW_KEY_E)) {
    cam->set_position(cam->position() -
                      (cam->GetUpVector() * speed_ * delta_time));
  }

  if (manager_->cursor_mode() == MouseCursorMode::DISABLED) {
    if (manager_->GetMousePosition().x != 0.f) {
      cam->SetYaw(cam->GetYaw() - (manager_->GetMousePosition().x *
                                   rotation_speed_ * delta_time));
    }
    if (manager_->GetMousePosition().y != 0.f) {
      cam->SetPitch(cam->GetPitch() - (manager_->GetMousePosition().y *
                                       rotation_speed_ * delta_time));
    }
  }
}

} // namespace szt
