#ifndef VKS_MESHESHEAP
#define VKS_MESHESHEAP

#include <EASTL/vector.h>
#include <cstdint>
#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_CXX11
#include <glm/glm.hpp>
#include <mesh.h>
#include <vulkan_buffer.h>

namespace vks {

struct Vertex;
class VertexSetup;
class VulkanDevice;

struct BuilderMesh {
  BuilderMesh(uint32_t Num_idxs, uint32_t Start_idx, uint32_t Mat_idx);

  uint32_t num_idxs;
  uint32_t mat_idx;
  uint32_t start_idx;
}; // struct BuilderMesh

class MeshesHeapBuilder {
public:
  MeshesHeapBuilder(const VertexSetup &vtx_setup, VkDescriptorPool desc_pool);

  // Returns false if this heap couldn't hold this mesh
  bool TestMesh(uint32_t num_vtxs, uint32_t num_idxs) const;

  void AddMesh(uint32_t mat_id, uint32_t num_idxs);

  void AddIndex(uint32_t index);
  void AddVertex(const Vertex &vertex);

  const eastl::vector<uint8_t> vertices_data(uint32_t i) const {
    return vertices_data_[i];
  }
  const eastl::vector<uint32_t> indices_data() const { return indices_data_; }
  const eastl::vector<Mesh> meshes() const { return meshes_; }
  uint32_t current_vertex() const { return current_vertex_; }
  const VertexSetup *vtx_setup() const { return vtx_setup_; }
  VkDescriptorPool desc_pool() const { return desc_pool_; }

private:
  eastl::vector<eastl::vector<uint8_t>> vertices_data_;
  eastl::vector<uint32_t> indices_data_;
  const VertexSetup *vtx_setup_;
  eastl::vector<Mesh> meshes_;
  uint32_t size_;
  uint32_t current_vertex_;
  VkDescriptorPool desc_pool_;

}; // class MeshesHeapBuilder

class MeshesHeap {
public:
  MeshesHeap(const VulkanDevice &device, const MeshesHeapBuilder &builder);
  ~MeshesHeap();

  void CreateAndWriteDescriptorSets(VkDescriptorSetLayout heap_set_layout);

  void BindVertexBuffer(VkCommandBuffer cmd_buff) const;
  void BindIndexBuffer(VkCommandBuffer cmd_buff) const;

  void Render(VkCommandBuffer cmd_buff, VkPipelineLayout pipe_layout,
              uint32_t desc_set_slot) const;

  uint32_t NumMeshes() const;

private:
  void CreateBuffers(const VulkanDevice &device,
                     const MeshesHeapBuilder &builder);
  void CreateDescriptorSet(VkDescriptorSetLayout heap_set_layout);
  void WriteDescriptorSet();

  eastl::vector<Mesh> meshes_;
  eastl::vector<VulkanBuffer> vertex_buffers_;
  VulkanBuffer index_buffer_;
  VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info_;
  eastl::vector<VkVertexInputBindingDescription> bindings_;
  eastl::vector<VkVertexInputAttributeDescription> attributes_;
  eastl::vector<VkDrawIndexedIndirectCommand> indirect_draw_cmds_;
  VulkanBuffer model_matxs_buff_;
  VulkanBuffer materialIDs_buff_;
  VulkanBuffer indirect_draw_buff_;
  VkDescriptorSet heap_desc_set_;
  VkDescriptorPool desc_pool_;
  const VertexSetup *vtx_setup_;
  const VulkanDevice *device_;

}; // class MeshesHeap

} // namespace vks

#endif
