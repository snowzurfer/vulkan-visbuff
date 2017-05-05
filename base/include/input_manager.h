#ifndef SZT_INPUTMANAGER
#define SZT_INPUTMANAGER

#define GLM_FORCE_CXX11
#include <GLFW/glfw3.h>
#include <cstdint>
#include <glm/glm.hpp>

namespace szt {

enum class MouseButton : uint8_t {
  LEFT = GLFW_MOUSE_BUTTON_LEFT,
  RIGHT = GLFW_MOUSE_BUTTON_RIGHT,
  MIDDLE = GLFW_MOUSE_BUTTON_MIDDLE,
  ButtonsCount = GLFW_MOUSE_BUTTON_LAST
}; // enum class MouseButton

enum class MouseCursorMode : int32_t {
  NORMAL = GLFW_CURSOR_NORMAL,
  DISABLED = GLFW_CURSOR_DISABLED,
  HIDDEN = GLFW_CURSOR_HIDDEN
}; // enum class MouseCursorMode

class InputManager {
public:
  InputManager();

  void Init(GLFWwindow *window);

  float mouse_x() const { return mouse_x_; }
  float mouse_y() const { return mouse_y_; }
  MouseCursorMode cursor_mode() const { return cursor_mode_; }
  bool IsKeyPressed(int32_t key) const { return keys_pressed_[key]; }
  bool IsKeyDown(int32_t key) const { return keys_down_[key]; }
  bool IsMouseButtonDown(MouseButton btn) const {
    return mouse_down_[static_cast<int32_t>(btn)];
  }
  bool IsMouseButtonPressed(MouseButton btn) const {
    return mouse_pressed_[static_cast<int32_t>(btn)];
  }

  /**
   * @brief Set position to 0.f, 0,f.
   */
  void ResetMousePosition(GLFWwindow *window);
  glm::vec2 GetMousePosition() const;
  void SetCursorMode(GLFWwindow *window, MouseCursorMode mode);
  /**
   * @brief To be called at end of main loop; resets pressed keys.
   */
  void EndFrame(GLFWwindow *window);

private:
  float mouse_x_;
  float mouse_y_;
  bool keys_down_[GLFW_KEY_LAST];
  bool keys_pressed_[GLFW_KEY_LAST];
  bool mouse_down_[GLFW_MOUSE_BUTTON_LAST];
  bool mouse_pressed_[GLFW_MOUSE_BUTTON_LAST];
  /**
   * @brief Used to check whether to clear x&y mouse pos at end of frame
   */
  MouseCursorMode cursor_mode_;

  static void GLFWKeyCallback(GLFWwindow *window, int32_t key, int32_t scancode,
                              int32_t action, int32_t mods);

  static void GLFWCursorPositionCallback(GLFWwindow *window, double x_pos,
                                         double y_pos);

  static void GLFWMouseButtonCallback(GLFWwindow *window, int32_t button,
                                      int32_t action, int32_t mods);

  void HandleKeyEvent(int32_t key_num, bool down);
  void HandleMouseMove(float x, float y);
  void HandleMouseEvent(int32_t btn_num, bool down);

}; // class InputManager

} // namespace szt

#endif
