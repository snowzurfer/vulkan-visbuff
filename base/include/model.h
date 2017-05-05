#ifndef VKS_MODEL
#define VKS_MODEL

#include <EASTL/vector.h>
#include <mesh.h>
#include <vulkan_buffer.h>
#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_CXX11
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <map>
#include <renderer_type.h>
#include <vertex_setup.h>
#include <vulkan_tools.h>

namespace vks {

struct Vertex {
  Vertex();

  glm::vec3 pos;
  glm::vec3 normal;
  glm::vec3 uv;
  glm::vec4 colour;
  glm::vec3 bitangent;
  glm::vec3 tangent;

  // Used by std::map to hash a vertex
  bool operator==(const Vertex &other) const {
    return (pos == other.pos && normal == other.normal && uv == other.uv &&
            colour == other.colour && bitangent == other.bitangent &&
            tangent == other.tangent);
  }

}; // struct Vertex

} // namespace vks

namespace std {

// Used by std::map to hash a Vertex
template <> struct hash<vks::Vertex> {
  size_t operator()(const vks::Vertex &vertex) const {
    return ((hash<glm::vec3>()(vertex.pos) ^
             (hash<glm::vec3>()(vertex.normal) << 1)) >>
            1) ^
           (hash<glm::vec2>()(vertex.uv) << 1);
  }
}; // template<> struct hash<vks::Vertex>

} // namespace std

namespace vks {

extern const uint32_t kModelMatsBindingPos;

class VulkanDevice;
class VertexSetup;

class ModelBuilder {
public:
  ModelBuilder(const VertexSetup &vertex_setup, VkDescriptorPool desc_pool);

  void AddIndex(uint32_t index);
  void AddVertex(const Vertex &vertex);
  void AddMesh(const Mesh *mesh);

  void AddVertexElementArray(const void *data, uint32_t size,
                             VertexElementType type);

  const eastl::vector<uint8_t> vertices_data(uint32_t i) const {
    return vertices_data_[i];
  }
  const eastl::vector<uint32_t> indices_data() const { return indices_data_; }
  const eastl::vector<const Mesh *> meshes() const { return meshes_; }
  uint32_t current_vertex() const { return current_vertex_; }
  uint32_t vertex_size() const { return vertex_size_; }
  const VertexSetup *vertex_setup() const { return vertex_setup_; }
  VkDescriptorPool desc_pool() const { return desc_pool_; }

private:
  eastl::vector<eastl::vector<uint8_t>> vertices_data_;
  eastl::vector<uint32_t> indices_data_;
  eastl::vector<const Mesh *> meshes_;
  uint32_t vertex_size_;
  uint32_t current_vertex_;
  const VertexSetup *vertex_setup_;
  VkDescriptorPool desc_pool_;
}; // class ModelBuilder

class Model {
public:
  Model();

  void Init(const VulkanDevice &device, const ModelBuilder &model_builder);
  void Shutdown(const VulkanDevice &device);

  void CreateAndWriteDescriptorSets(const VulkanDevice &device,
                                    VkDescriptorSetLayout heap_set_layout);

  const VkPipelineVertexInputStateCreateInfo &
  vertex_input_state_create_info() const {
    return vertex_input_state_create_info_;
  }

  const eastl::vector<Mesh> &meshes() const { return meshes_; }
  uint32_t GetMeshesCount() const { return SCAST_U32(meshes_.size()); }

  void BindVertexBuffer(VkCommandBuffer cmd_buff) const;
  void BindIndexBuffer(VkCommandBuffer cmd_buff) const;

  void RenderMeshesByMaterial(VkCommandBuffer cmd_buff,
                              VkPipelineLayout pipe_layout,
                              uint32_t desc_set_slot) const;

  uint32_t NumMeshes() const;

  /**
   * @brief SetModelMatrixForAllMeshes Set a specific model matrix for all the
   *   meshes in the model.
   *
   * @param mat The matrix to set.
   */
  void SetModelMatrixForAllMeshes(const glm::mat4 &mat);

private:
  void CreateBuffers(const VulkanDevice &device, const ModelBuilder &builder);
  void CreateDescriptorSet(const VulkanDevice &device,
                           VkDescriptorSetLayout heap_set_layout);
  void WriteDescriptorSet(const VulkanDevice &device);

  eastl::vector<Mesh> meshes_;
  eastl::vector<VulkanBuffer> vertex_buffers_;
  VulkanBuffer index_buffer_;
  VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info_;
  eastl::vector<VkVertexInputBindingDescription> bindings_;
  eastl::vector<VkVertexInputAttributeDescription> attributes_;
  VulkanBuffer model_matxs_buff_;
  VulkanBuffer materialIDs_buff_;
  VulkanBuffer indirect_draws_buff_;
  VkDescriptorSet desc_set_;
  VkDescriptorPool desc_pool_;
  VertexSetup vtx_setup_;

}; // class Model

} // namespace vks

#endif
