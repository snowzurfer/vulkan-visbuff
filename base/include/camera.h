/**
 * @file camera.h
 * @brief Camera class
 * @author Alberto Taiuti
 * @version 1.0
 * @date 2016-09-07
 */

#ifndef SZT_CAMERA
#define SZT_CAMERA

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <frustum.h>
#include <glm/glm.hpp>
#include <viewport.h>

namespace szt {

class Camera {
public:
  Camera();

  void Init(const Viewport &viewport, const Frustum &frustum);

  void set_position(const glm::vec3 &position) {
    position_ = position;
    MarkDirty();
  }
  void set_rotation(const glm::vec3 &rotation) {
    rotation_ = rotation;
    MarkDirty();
  }

  // Angle is in degrees
  void SetYaw(float yaw);
  // Angle is in degrees
  void SetPitch(float pitch);
  // Angle is in degrees
  void SetRoll(float roll);

  float GetYaw() const { return rotation_.y; }
  float GetPitch() const { return rotation_.x; }
  float GetRoll() const { return rotation_.z; }

  const glm::vec3 &rotation() const { return rotation_; }
  const glm::vec3 &position() const { return position_; }

  glm::vec3 GetForwardVector() const;
  glm::vec3 GetRightVector() const;
  glm::vec3 GetUpVector() const;

  const glm::mat4 &view_mat() const {
    CheckDirty();
    return view_mat_;
  }
  const glm::mat4 &projection_mat() const { return projection_mat_; }

  void SetPerspectiveMatrix(float fov_y, float aspect_ratio, float near,
                            float far);

  const Viewport &viewport() const { return viewport_; }
  const Frustum &frustum() const { return frustum_; }

private:
  glm::vec3 position_;
  glm::vec3 rotation_;
  mutable bool recalculate_mat_;
  mutable glm::mat4 view_mat_;
  glm::mat4 projection_mat_;
  szt::Viewport viewport_;
  szt::Frustum frustum_;

  // Used to flip the y
  glm::mat4 gl_y_to_vulkan_y_mat_;

  // Used to lazily update matrix
  void MarkDirty() { recalculate_mat_ = true; }
  void CheckDirty() const;

}; // class Camera

} // namespace szt

#endif
