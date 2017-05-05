#ifndef VKS_MESH
#define VKS_MESH

#define GLM_FORCE_CXX11
#include <cstdint>
#include <glm/glm.hpp>

namespace vks {

class Mesh {
public:
  Mesh();
  Mesh(uint32_t start_index, uint32_t index_count, uint32_t vertex_offset,
       uint32_t material_id);

  uint32_t start_index() const { return start_index_; }
  uint32_t index_count() const { return index_count_; }
  uint32_t vertex_offset() const { return vertex_offset_; }
  uint32_t material_id() const { return material_id_; }
  const glm::mat4 &model_mat() const { return model_mat_; }
  uint32_t dynamic_ubo_offset() const { return dynamic_ubo_offset_; }

  void set_model_mat(const glm::mat4 &mat) { model_mat_ = mat; }
  void set_dynamic_ubo_offset(const uint32_t offset) {
    dynamic_ubo_offset_ = offset;
  }

private:
  uint32_t start_index_;
  uint32_t index_count_;
  uint32_t vertex_offset_;
  // ID of the material this mesh uses, as stored in the material manager
  uint32_t material_id_;
  glm::mat4 model_mat_;
  // The offset within the model's dynamic ubo for the model mat of this
  // mesh
  uint32_t dynamic_ubo_offset_;

}; // class Mesh

} // namespace vks

#endif
