#include <cstring>
#include <input_manager.h>

namespace szt {

InputManager::InputManager()
    : mouse_x_(0), mouse_y_(0), keys_down_(), keys_pressed_(), mouse_down_(),
      mouse_pressed_(), cursor_mode_(MouseCursorMode::NORMAL) {
  std::memset(keys_pressed_, 0U, sizeof(keys_pressed_));
  std::memset(keys_down_, 0U, sizeof(keys_down_));
  std::memset(mouse_pressed_, 0U, sizeof(mouse_pressed_));
  std::memset(mouse_down_, 0U, sizeof(mouse_down_));
}

void InputManager::Init(GLFWwindow *window) {
  glfwSetKeyCallback(window, InputManager::GLFWKeyCallback);
  glfwSetCursorPosCallback(window, InputManager::GLFWCursorPositionCallback);
  glfwSetMouseButtonCallback(window, InputManager::GLFWMouseButtonCallback);
  // Used to retrieve the input manager instance within the static callback
  // funcs
  glfwSetWindowUserPointer(window, this);
}

void InputManager::HandleKeyEvent(int32_t key_num, bool down) {
  keys_down_[key_num] = keys_pressed_[key_num] = down;
}

void InputManager::HandleMouseMove(float x, float y) {
  mouse_x_ = x;
  mouse_y_ = y;
}

void InputManager::HandleMouseEvent(int32_t btn_num, bool down) {
  mouse_down_[btn_num] = mouse_pressed_[btn_num] = down;
}

void InputManager::EndFrame(GLFWwindow *window) {
  std::memset(keys_pressed_, 0U, sizeof(keys_pressed_));
  std::memset(mouse_pressed_, 0U, sizeof(mouse_pressed_));

  ResetMousePosition(window);
}

void InputManager::GLFWKeyCallback(GLFWwindow *window, int32_t key,
                                   int32_t scancode, int32_t action,
                                   int32_t mods) {
  InputManager *manager =
      static_cast<InputManager *>(glfwGetWindowUserPointer(window));

  if (action == static_cast<int32_t>(GLFW_PRESS) ||
      static_cast<int32_t>(action == GLFW_RELEASE)) {
    manager->HandleKeyEvent(key, static_cast<bool>(action));
  }
}

void InputManager::GLFWCursorPositionCallback(GLFWwindow *window, double x_pos,
                                              double y_pos) {
  InputManager *manager =
      static_cast<InputManager *>(glfwGetWindowUserPointer(window));

  manager->HandleMouseMove(static_cast<float>(x_pos),
                           static_cast<float>(y_pos));
}

void InputManager::GLFWMouseButtonCallback(GLFWwindow *window, int32_t button,
                                           int32_t action, int32_t mods) {
  InputManager *manager =
      static_cast<InputManager *>(glfwGetWindowUserPointer(window));

  manager->HandleMouseEvent(button, static_cast<bool>(action));
}

void InputManager::ResetMousePosition(GLFWwindow *window) {
  mouse_x_ = mouse_y_ = 0.f;
  glfwSetCursorPos(window, static_cast<double>(mouse_x_),
                   static_cast<double>(mouse_y_));
}

glm::vec2 InputManager::GetMousePosition() const {
  return glm::vec2(mouse_x_, mouse_y_);
}

void InputManager::SetCursorMode(GLFWwindow *window, MouseCursorMode mode) {
  glfwSetInputMode(window, GLFW_CURSOR, static_cast<int32_t>(mode));
  cursor_mode_ = mode;
}

} // namespace szt
