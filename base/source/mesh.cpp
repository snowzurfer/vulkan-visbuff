#include <mesh.h>

namespace vks {

Mesh::Mesh()
    : start_index_(0U), index_count_(0U), vertex_offset_(0U), material_id_(0U),
      model_mat_(1.f), dynamic_ubo_offset_(0U) {}

Mesh::Mesh(uint32_t start_index, uint32_t index_count, uint32_t vertex_offset,
           uint32_t material_id)
    : start_index_(start_index), index_count_(index_count),
      vertex_offset_(vertex_offset), material_id_(material_id), model_mat_(1.f),
      dynamic_ubo_offset_(0U) {}

} // namespace vks
