#include <camera.h>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.inl>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace szt {

Camera::Camera()
    : position_(0.f), rotation_(0.f, 90.f, 0.f), recalculate_mat_(false),
      view_mat_(1.f), projection_mat_(1.f), viewport_(), frustum_(),
      gl_y_to_vulkan_y_mat_(1.f) {}

void Camera::Init(const Viewport &viewport, const Frustum &frustum) {
  viewport_ = viewport;
  frustum_ = frustum;

  gl_y_to_vulkan_y_mat_[1U].y = -1.f;

  SetPerspectiveMatrix(frustum_.fov_y(),
                       static_cast<float>(viewport_.width) /
                           static_cast<float>(viewport_.height),
                       frustum_.near(), frustum_.far());

  position_ = glm::vec3(0.f, 2.f, 4.f);

  MarkDirty();
  CheckDirty();
}

glm::vec3 Camera::GetForwardVector() const {
  float yaw = glm::radians(rotation_.y);
  float pitch = glm::radians(rotation_.x);

  float cos_y = cosf(yaw);
  float cos_p = cosf(pitch);
  float sin_y = sinf(yaw);
  float sin_p = sinf(pitch);
  return glm::normalize(glm::vec3(cos_y * cos_p, sin_p, -(sin_y * cos_p)));
}

glm::vec3 Camera::GetUpVector() const { return glm::vec3(0.f, 1.f, 0.f); }

glm::vec3 Camera::GetRightVector() const {
  return glm::cross(GetForwardVector(), GetUpVector());
}

void Camera::CheckDirty() const {
  if (recalculate_mat_) {
    glm::vec3 up = GetUpVector();
    glm::vec3 forward = GetForwardVector();
    view_mat_ = glm::lookAt(position_, forward + position_, up);
    recalculate_mat_ = false;
  }
}

void Camera::SetPerspectiveMatrix(float fov_y, float aspect_ratio, float near,
                                  float far) {
  projection_mat_ =
      glm::perspective(glm::radians(fov_y), aspect_ratio, near, far);
  projection_mat_ = gl_y_to_vulkan_y_mat_ * projection_mat_;
}

void Camera::SetYaw(float yaw) {
  if (yaw > 360.f) {
    yaw = 0.f;
  }
  if (yaw < -360.f) {
    yaw = 0.f;
  }

  rotation_.y = yaw;
  MarkDirty();
}

void Camera::SetPitch(float pitch) {
  if (pitch > 89.f) {
    pitch = 89.f;
  }
  if (pitch < -89.f) {
    pitch = -89.f;
  }

  rotation_.x = pitch;
  MarkDirty();
}

void Camera::SetRoll(float roll) {
  rotation_.z = roll;
  MarkDirty();
}

} // namespace szt
