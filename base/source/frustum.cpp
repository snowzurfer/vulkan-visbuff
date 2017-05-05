#include <cmath>
#include <frustum.h>
#include <glm/trigonometric.hpp>

namespace szt {

Frustum::Frustum()
    : near_size_(), far_size_(), near_(0.f), far_(1.f), fov_y_(45.f),
      aspect_ratio_(2.f), ftl_(), ftr_(), fbl_(), fbr_() {}

Frustum::Frustum(float near, float far, float fov_y, float aspect_ratio)
    : near_size_(), far_size_(), near_(near), far_(far), fov_y_(fov_y),
      aspect_ratio_(aspect_ratio), ftl_(), ftr_(), fbl_(), fbr_() {
  // Calculate height and width of far and near plane
  float half_tanf = std::tan(glm::radians(fov_y_) / 2.f);
  near_size_.y = 2.f * half_tanf * near_;
  near_size_.x = near_size_.y * aspect_ratio_;
  far_size_.y = 2.f * half_tanf * far_;
  far_size_.x = far_size_.y * aspect_ratio_;

  // Calculate the view space positions of the corners of the far plane
  glm::vec2 half_sizes_far(far_size_.x / 2.f, far_size_.y / 2.f);
  glm::vec3 far_centre(0.f, 0.f, -far_);
  glm::vec3 up(0.f, 1.f, 0.f);
  glm::vec3 right(1.f, 0.f, 0.f);

  ftl_ = far_centre + (up * half_sizes_far.y) - (right * half_sizes_far.x);
  ftr_ = far_centre + (up * half_sizes_far.y) + (right * half_sizes_far.x);
  fbl_ = far_centre - (up * half_sizes_far.y) - (right * half_sizes_far.x);
  fbr_ = far_centre - (up * half_sizes_far.y) + (right * half_sizes_far.x);
}

} // namespace szt
