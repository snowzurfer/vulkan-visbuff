#ifndef VKS_FRUSTUM
#define VKS_FRUSTUM

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace szt {

class Frustum {
public:
  Frustum();
  Frustum(float near, float far, float fov_y, float aspect_ratio);

  const glm::vec2 &near_size() const { return near_size_; }
  const glm::vec2 &far_size() const { return far_size_; }
  const glm::vec3 &ftl() const { return ftl_; }
  const glm::vec3 &ftr() const { return ftr_; }
  const glm::vec3 &fbr() const { return fbr_; }
  const glm::vec3 &fbl() const { return fbl_; }
  float fov_y() const { return fov_y_; }
  float far() const { return far_; }
  float near() const { return near_; }

private:
  glm::vec2 near_size_;
  glm::vec2 far_size_;
  float near_;
  float far_;
  float fov_y_;
  float aspect_ratio_;
  glm::vec3 ftl_;
  glm::vec3 ftr_;
  glm::vec3 fbl_;
  glm::vec3 fbr_;

}; // class Frustum

} // namespace szt

#endif
