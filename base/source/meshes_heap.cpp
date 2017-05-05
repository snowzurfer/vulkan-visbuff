#include <meshes_heap.h>
#include <vertex_setup.h>
#include <vulkan_device.h>
#include <vulkan_tools.h>
#include <glm/gtc/type_ptr.hpp>
#include <cstring>
#include <model.h>
#include <EASTL/sort.h>
#include <base_system.h>
#include <logger.hpp>

namespace vks {

// 64 MB, not MiB!
const uint32_t kHeapMaxSize = 64U * 1000000U;
extern const uint32_t kVertexBuffersBaseBindPos = 4U;
extern const uint32_t kIndirectDrawCmdsBindingPos = 3U;
extern const uint32_t kIdxBufferBindPos = 2U;
extern const uint32_t kModelMatxsBufferBindPos = 0U;
extern const uint32_t kMaterialIDsBufferBindPos = 1U;

MeshesHeapBuilder::MeshesHeapBuilder(
    const VertexSetup &vtx_setup,
    VkDescriptorPool desc_pool)
    : vertices_data_(vtx_setup.num_elements()),
      indices_data_(),
      vtx_setup_(&vtx_setup),
      meshes_(),
      size_(0U),
      current_vertex_(0U),
      desc_pool_(desc_pool) {}

bool MeshesHeapBuilder::TestMesh(
    uint32_t num_vtxs,
    uint32_t num_idxs) const {
  if ((size_ + (num_vtxs * vtx_setup_->vertex_size()) +
      (num_idxs * SCAST_U32(sizeof(uint32_t)))) > kHeapMaxSize) {
    return false;
  }

  return true;
}

void MeshesHeapBuilder::AddMesh(
    uint32_t mat_id,
    uint32_t num_idxs) {
  meshes_.push_back(Mesh(SCAST_U32(indices_data_.size()), num_idxs, 0U,
                         mat_id));
}


void MeshesHeapBuilder::AddIndex(uint32_t index) {
  indices_data_.push_back(index);
}

void MeshesHeapBuilder::AddVertex(const Vertex &vertex) {
  // Use a vector to layout the data in memory as specified by the layout 
  uint32_t elm_idx = 0U;
  for (eastl::vector<eastl::vector<uint8_t>>::iterator i =
         vertices_data_.begin();
       i != vertices_data_.end();
       ++i, ++elm_idx) {
    uint32_t element_size = vtx_setup_->GetElementSize(elm_idx);
    if (i->size() < (current_vertex_ + 1U) * element_size) {
      i->resize(i->size() + element_size);
    }

    unsigned char *dst =
      i->data() +
      (element_size * current_vertex_);

    const void *read_location = nullptr;
    switch (vtx_setup_->vertex_types_layout()[elm_idx]) {
      case VertexElementType::POSITION: {
        read_location = SCAST_CVOIDPTR(glm::value_ptr(vertex.pos));
        break;
      }
      case VertexElementType::NORMAL: {
        read_location = SCAST_CVOIDPTR(glm::value_ptr(vertex.normal));
        break;
      }
      case VertexElementType::COLOUR: {
        read_location = SCAST_CVOIDPTR(glm::value_ptr(vertex.colour));
        break;
      }
      case VertexElementType::UV: {
        read_location = SCAST_CVOIDPTR(glm::value_ptr(vertex.uv));
        break;
      }
      case VertexElementType::TANGENT: {
        read_location = SCAST_CVOIDPTR(glm::value_ptr(vertex.tangent));
        break;
      }
      case VertexElementType::BITANGENT: {
        read_location = SCAST_CVOIDPTR(glm::value_ptr(vertex.bitangent));
        break;
      }
      default:
        ELOG_WARN("Unsupported vertex element type!");
    }

    memcpy(
        dst,
        read_location,
        element_size);
  } 

  current_vertex_++;
}

MeshesHeap::MeshesHeap(const VulkanDevice &device,
  const MeshesHeapBuilder &builder)
  : meshes_(),
    vertex_buffers_(),
    index_buffer_(),
    vertex_input_state_create_info_(
        tools::inits::PipelineVertexInputStateCreateInfo()),
    bindings_(),
    attributes_(),
    indirect_draw_cmds_(),
    model_matxs_buff_(),
    materialIDs_buff_(),
    indirect_draw_buff_(),
    heap_desc_set_(VK_NULL_HANDLE),
    desc_pool_(builder.desc_pool()),
    vtx_setup_(builder.vtx_setup()),
    device_(&device) {
  uint32_t meshes_count = SCAST_U32(builder.meshes().size());

  for (uint32_t i = 0U; i < meshes_count; i++) {
    meshes_.push_back((builder.meshes()[i]));
  }
  
  eastl::sort(meshes_.begin(), meshes_.end(),
    [](const Mesh &lhs, const Mesh &rhs) {
    return (lhs.material_id() < rhs.material_id());
  });

  CreateBuffers(device, builder);
}
  
MeshesHeap::~MeshesHeap() {
  index_buffer_.Shutdown(*device_);
  for (eastl::vector<VulkanBuffer>::iterator i = vertex_buffers_.begin();
       i != vertex_buffers_.end();
       ++i) {
    i->Shutdown(*device_);
  }
  model_matxs_buff_.Shutdown(*device_);
  materialIDs_buff_.Shutdown(*device_);
  indirect_draw_buff_.Shutdown(*device_);
}

void MeshesHeap::CreateBuffers(const VulkanDevice &device,
                               const MeshesHeapBuilder &builder) {
  uint32_t meshes_count = SCAST_U32(builder.meshes().size());
  
  // Create buffers for the vertex and index buffers
  VulkanBufferInitInfo init_info;
  
  vertex_buffers_.resize(builder.vtx_setup()->num_elements());
  uint32_t elm_idx = 0U;
  for (eastl::vector<VulkanBuffer>::iterator i = vertex_buffers_.begin();
       i != vertex_buffers_.end();
       ++i, ++elm_idx) { 
    init_info.buffer_usage_flags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    init_info.memory_property_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    init_info.size = SCAST_U32(builder.vertices_data(elm_idx).size()) *
      SCAST_U32(sizeof(uint8_t)),
    init_info.cmd_buff = vulkan()->copy_cmd_buff();
    i->Init(
        device,
        init_info, 
        SCAST_CVOIDPTR(builder.vertices_data(elm_idx).data()));
    LOG("BUFF: " << i->buffer());
  }

  init_info.size = SCAST_U32(builder.indices_data().size()) *
    SCAST_U32(sizeof(uint32_t));
  init_info.buffer_usage_flags = VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
  index_buffer_.Init(
      device,
      init_info, 
      SCAST_CVOIDPTR(builder.indices_data().data()));

  // Create model matrices buffer
  init_info.size = meshes_count * SCAST_U32(sizeof(glm::mat4));
  init_info.memory_property_flags = /*VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |*/
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
  init_info.buffer_usage_flags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
  model_matxs_buff_.Init(device, init_info);

  // Upload the data to it
  uint32_t mat4_size = SCAST_U32(sizeof(glm::mat4));
  uint32_t counter = 0U;
  for (eastl::vector<Mesh>::iterator itor = meshes_.begin();
       itor != meshes_.end();
       ++itor, ++counter) { 
      void *data = nullptr;
      model_matxs_buff_.Map(device, &data, mat4_size, counter * mat4_size);
      *static_cast<glm::mat4 *>(data) = itor->model_mat();
      model_matxs_buff_.Unmap(device);
  }
 
  // Create the materials ID buffer
  init_info.size = SCAST_U32(sizeof(uint32_t)) *
    meshes_count;
  init_info.buffer_usage_flags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

  materialIDs_buff_.Init(device, init_info);

  // Upload data to it
  eastl::vector<uint32_t> material_ids(meshes_count);
  eastl::vector<Mesh>::iterator m_itor = meshes_.begin();
  for (eastl::vector<uint32_t>::iterator itor = material_ids.begin();
       itor != material_ids.end();
       ++itor, ++m_itor) { 
    *itor = m_itor->material_id();
  }
  void *mapped_memory = nullptr; 
  materialIDs_buff_.Map(
      vulkan()->device(),
      &mapped_memory,
      SCAST_U32(materialIDs_buff_.size()));
  memcpy(mapped_memory, SCAST_CVOIDPTR(material_ids.data()),
      SCAST_U32(material_ids.size()) *
        SCAST_U32(sizeof(uint32_t)));
  materialIDs_buff_.Unmap(vulkan()->device());
 
  // Setup indirect draw buffers
  init_info.size = SCAST_U32(sizeof(VkDrawIndexedIndirectCommand)) *
    meshes_count;
  VKS_ASSERT(init_info.size > 0U, "Size of init_info is zero!");
  init_info.memory_property_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
    /*VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |*/ VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
  init_info.buffer_usage_flags = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT |
    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

  indirect_draw_buff_.Init(device, init_info); 
 
  // Upload data to it
  m_itor = meshes_.begin();
  indirect_draw_cmds_.resize(meshes_count);
  for (eastl::vector<VkDrawIndexedIndirectCommand>::iterator itor = 
         indirect_draw_cmds_.begin();
       itor != indirect_draw_cmds_.end();
       ++itor, ++m_itor) {
    itor->indexCount = m_itor->index_count(); 
    itor->instanceCount = 1U;
    itor->firstIndex = m_itor->start_index();
    itor->vertexOffset = m_itor->vertex_offset();
    itor->firstInstance = 0U;
  }
  indirect_draw_buff_.Map(
      vulkan()->device(),
      &mapped_memory, 
      SCAST_U32(indirect_draw_cmds_.size()) *
        SCAST_U32(sizeof(VkDrawIndexedIndirectCommand)));
  memcpy(mapped_memory, SCAST_CVOIDPTR(indirect_draw_cmds_.data()),
     SCAST_U32(indirect_draw_cmds_.size()) *
      SCAST_U32(sizeof(VkDrawIndexedIndirectCommand)));
  indirect_draw_buff_.Unmap(vulkan()->device());
}

void MeshesHeap::CreateDescriptorSet(VkDescriptorSetLayout heap_set_layout) {
  LOG("DESCPOOOL: " << desc_pool_);
  // Create descriptor set for this heap
  VkDescriptorSetAllocateInfo set_allocate_info =
    tools::inits::DescriptorSetAllocateInfo(
      desc_pool_,
      1U,
      &heap_set_layout);

  VK_CHECK_RESULT(vkAllocateDescriptorSets(
        device_->device(),
        &set_allocate_info,
        &heap_desc_set_));  
}

void MeshesHeap::WriteDescriptorSet() {
  eastl::vector<VkWriteDescriptorSet> write_desc_sets;

  uint32_t counter = 0U;
  eastl::vector<VkDescriptorBufferInfo> elems_buff_infos(
      vertex_buffers_.size());
  for (eastl::vector<VulkanBuffer>::iterator itor = vertex_buffers_.begin();
       itor != vertex_buffers_.end();
       ++itor, ++counter) {
    elems_buff_infos[counter] =
      itor->GetDescriptorBufferInfo();
    uint32_t pos = kVertexBuffersBaseBindPos +
          vtx_setup_->GetElementPosition(counter);
    write_desc_sets.push_back(tools::inits::WriteDescriptorSet(
        heap_desc_set_,
        pos,
        0U,
        1U,
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        nullptr,
        &elems_buff_infos[counter],
        nullptr));
  }

  VkDescriptorBufferInfo idx_buff_info =
    index_buffer_.GetDescriptorBufferInfo();
  write_desc_sets.push_back(tools::inits::WriteDescriptorSet(
      heap_desc_set_,
      kIdxBufferBindPos,
      0U,
      1U,
      VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
      nullptr,
      &idx_buff_info,
      nullptr));
  
  VkDescriptorBufferInfo model_matxs_buff_info =
    model_matxs_buff_.GetDescriptorBufferInfo();
  write_desc_sets.push_back(tools::inits::WriteDescriptorSet(
      heap_desc_set_,
      kModelMatxsBufferBindPos,
      0U,
      1U,
      VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
      nullptr,
      &model_matxs_buff_info,
      nullptr));
  
  VkDescriptorBufferInfo materialIDs_buff_info = 
    materialIDs_buff_.GetDescriptorBufferInfo();
  write_desc_sets.push_back(tools::inits::WriteDescriptorSet(
      heap_desc_set_,
      kMaterialIDsBufferBindPos,
      0U,
      1U,
      VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
      nullptr,
      &materialIDs_buff_info,
      nullptr));
  
  VkDescriptorBufferInfo desc_indirect_draw_buff_info =
    indirect_draw_buff_.GetDescriptorBufferInfo();
  write_desc_sets.push_back(tools::inits::WriteDescriptorSet(
      heap_desc_set_,
      kIndirectDrawCmdsBindingPos,
      0U,
      1U,
      VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
      nullptr,
      &desc_indirect_draw_buff_info,
      nullptr));

  vkUpdateDescriptorSets(
      device_->device(),
      SCAST_U32(write_desc_sets.size()),
      write_desc_sets.data(),
      0U,
      nullptr);
}


void MeshesHeap::BindVertexBuffer(VkCommandBuffer cmd_buff) const {
  uint32_t num_elements = SCAST_U32(vertex_buffers_.size());
  eastl::vector<VkDeviceSize> offsets;
  offsets.assign(num_elements, 0U);
  eastl::vector<VkBuffer> buffers(num_elements);
  eastl::vector<VkBuffer>::iterator bi = buffers.begin();
  for (eastl::vector<VulkanBuffer>::const_iterator i = vertex_buffers_.begin();
       i != vertex_buffers_.end();
       ++i, ++bi) {
    *bi = i->buffer();
  }

  vkCmdBindVertexBuffers(
      cmd_buff,
      0U,
      num_elements,
      buffers.data(),
      offsets.data());
}

void MeshesHeap::BindIndexBuffer(VkCommandBuffer cmd_buff) const {
  vkCmdBindIndexBuffer(
      cmd_buff,
      index_buffer_.buffer(),
      0U,
      VK_INDEX_TYPE_UINT32);
}

void MeshesHeap::Render(
      VkCommandBuffer cmd_buff,
      VkPipelineLayout pipe_layout,
      uint32_t desc_set_slot) const {
  // Set descriptor set
  vkCmdBindDescriptorSets(
    cmd_buff,
    VK_PIPELINE_BIND_POINT_GRAPHICS,
    pipe_layout,
    desc_set_slot,
    1U,
    &heap_desc_set_,
    0U,
    nullptr);

  // Render with the indirect buffer
  vkCmdDrawIndexedIndirect(
      cmd_buff,
      indirect_draw_buff_.buffer(),
      0U,
      SCAST_U32(indirect_draw_cmds_.size()),
      SCAST_U32(sizeof(VkDrawIndexedIndirectCommand)));
}

void MeshesHeap::CreateAndWriteDescriptorSets(
    VkDescriptorSetLayout heap_set_layout) {
  CreateDescriptorSet(heap_set_layout);

  WriteDescriptorSet();
}
  
uint32_t MeshesHeap::NumMeshes() const {
  return SCAST_U32(meshes_.size());
}

} // namespace vks
