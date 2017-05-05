#include <EASTL/vector.h>
#include <algorithm>
#include <base_system.h>
#include <cstring>
#include <deferred_renderer.h>
#include <deque>
#include <functional>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <logger.hpp>
#include <material_texture_type.h>
#include <model.h>
#include <queue>
#include <vulkan_device.h>
#include <vulkan_tools.h>

namespace vks {

extern const uint32_t kVertexBuffersBaseBindPos;
extern const uint32_t kIndirectDrawCmdsBindingPos;
extern const uint32_t kIdxBufferBindPos;
extern const uint32_t kModelMatxsBufferBindPos;
extern const uint32_t kMaterialIDsBufferBindPos;

Vertex::Vertex()
    : pos(0.f), normal(0.f), uv(0.f), colour(0.f), bitangent(0.f),
      tangent(0.f) {}

ModelBuilder::ModelBuilder(const VertexSetup &vertex_setup,
                           VkDescriptorPool desc_pool)
    : vertices_data_(vertex_setup.num_elements()), indices_data_(), meshes_(),
      vertex_size_(vertex_setup.vertex_size()), current_vertex_(0U),
      vertex_setup_(&vertex_setup), desc_pool_(desc_pool) {}

void ModelBuilder::AddIndex(uint32_t index) { indices_data_.push_back(index); }

void AddVertexElementArray(const void *data, uint32_t size,
                           VertexElementType type) {}

void ModelBuilder::AddVertex(const Vertex &vertex) {

  // Use a vector to layout the data in memory as specified by the layout
  // uint32_t layout_count = vertex_setup_->num_elements();
  uint32_t elm_idx = 0U;
  for (eastl::vector<eastl::vector<uint8_t>>::iterator
           i = vertices_data_.begin();
       i != vertices_data_.end(); ++i, ++elm_idx) {
    uint32_t element_size = vertex_setup_->GetElementSize(elm_idx);
    if (i->size() < (current_vertex_ + 1U) * element_size) {
      i->resize(i->size() + element_size);
    }

    unsigned char *dst = i->data() + (element_size * current_vertex_);

    const void *read_location = nullptr;
    switch (vertex_setup_->vertex_types_layout()[elm_idx]) {
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

    memcpy(dst, read_location, element_size);
  }

  // unsigned char *dst = vertices_data_.data() + (vertex_size_ *
  // current_vertex_);
  current_vertex_++;
}

void ModelBuilder::AddMesh(const Mesh *mesh) { meshes_.push_back(mesh); }

Model::Model()
    : meshes_(), vertex_buffers_(), index_buffer_(),
      vertex_input_state_create_info_(
          tools::inits::PipelineVertexInputStateCreateInfo()),
      bindings_(), attributes_(), model_matxs_buff_(), materialIDs_buff_(),
      indirect_draws_buff_(), desc_set_(VK_NULL_HANDLE),
      desc_pool_(VK_NULL_HANDLE), vtx_setup_() {}

void Model::Init(const VulkanDevice &device,
                 const ModelBuilder &model_builder) {
  uint32_t meshes_count = SCAST_U32(model_builder.meshes().size());

  for (uint32_t i = 0U; i < meshes_count; i++) {
    meshes_.push_back(*(model_builder.meshes()[i]));
  }

  // std::sort(meshes_.begin(), meshes_.end(),
  //  [](const Mesh &lhs, const Mesh &rhs) {
  //  return (lhs.material_id() < rhs.material_id());
  //});

  vtx_setup_ = *model_builder.vertex_setup();
  desc_pool_ = model_builder.desc_pool();

  CreateBuffers(device, model_builder);
}

void Model::CreateBuffers(const VulkanDevice &device,
                          const ModelBuilder &builder) {
  uint32_t meshes_count = SCAST_U32(builder.meshes().size());

  // Create buffers for the vertex and index buffers
  VulkanBufferInitInfo init_info;

  vertex_buffers_.resize(builder.vertex_setup()->num_elements());
  uint32_t elm_idx = 0U;
  for (eastl::vector<VulkanBuffer>::iterator i = vertex_buffers_.begin();
       i != vertex_buffers_.end(); ++i, ++elm_idx) {
    init_info.buffer_usage_flags =
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    init_info.memory_property_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    init_info.size = SCAST_U32(builder.vertices_data(elm_idx).size()) *
                     SCAST_U32(sizeof(uint8_t)),
    init_info.cmd_buff = vulkan()->copy_cmd_buff();
    i->Init(device, init_info,
            SCAST_CVOIDPTR(builder.vertices_data(elm_idx).data()));
  }

  init_info.size =
      SCAST_U32(builder.indices_data().size()) * SCAST_U32(sizeof(uint32_t));
  init_info.buffer_usage_flags =
      VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
  index_buffer_.Init(device, init_info,
                     SCAST_CVOIDPTR(builder.indices_data().data()));

  // Create model matrices buffer
  init_info.size = meshes_count * SCAST_U32(sizeof(glm::mat4));
  init_info.memory_property_flags = /*VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |*/
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
  init_info.buffer_usage_flags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
  model_matxs_buff_.Init(device, init_info);

  // Upload the data to it
  uint32_t mat4_size = SCAST_U32(sizeof(glm::mat4));
  uint32_t counter = 0U;
  for (eastl::vector<Mesh>::iterator itor = meshes_.begin();
       itor != meshes_.end(); ++itor, ++counter) {
    void *data = nullptr;
    model_matxs_buff_.Map(device, &data, mat4_size, counter * mat4_size);
    *static_cast<glm::mat4 *>(data) = itor->model_mat();
    model_matxs_buff_.Unmap(device);
  }

  // Create the materials ID buffer
  init_info.size = SCAST_U32(sizeof(uint32_t)) * meshes_count;
  init_info.buffer_usage_flags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

  materialIDs_buff_.Init(device, init_info);

  // Upload data to it
  eastl::vector<uint32_t> material_ids(meshes_count);
  eastl::vector<Mesh>::iterator m_itor = meshes_.begin();
  for (eastl::vector<uint32_t>::iterator itor = material_ids.begin();
       itor != material_ids.end(); ++itor, ++m_itor) {
    *itor = m_itor->material_id();
    LOG("MAT ID: " << *itor);
  }
  void *mapped_memory = nullptr;
  materialIDs_buff_.Map(vulkan()->device(), &mapped_memory);
  memcpy(mapped_memory, SCAST_CVOIDPTR(material_ids.data()),
         SCAST_U32(material_ids.size()) * SCAST_U32(sizeof(uint32_t)));
  materialIDs_buff_.Unmap(vulkan()->device());

  // Setup indirect draw buffers
  init_info.size =
      SCAST_U32(sizeof(VkDrawIndexedIndirectCommand)) * meshes_count;
  VKS_ASSERT(init_info.size > 0U, "Size of init_info is zero!");
  init_info.memory_property_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                    /*VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |*/
                                    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
  init_info.buffer_usage_flags =
      VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

  indirect_draws_buff_.Init(device, init_info);

  // Upload data to it
  m_itor = meshes_.begin();
  eastl::vector<VkDrawIndexedIndirectCommand> indirect_draw_cmds(meshes_count);
  for (eastl::vector<VkDrawIndexedIndirectCommand>::iterator
           itor = indirect_draw_cmds.begin();
       itor != indirect_draw_cmds.end(); ++itor, ++m_itor) {
    LOG("idx count: " << m_itor->index_count());
    itor->indexCount = m_itor->index_count();
    itor->instanceCount = 1U;
    LOG("start count: " << m_itor->start_index());
    itor->firstIndex = m_itor->start_index();
    itor->vertexOffset = m_itor->vertex_offset();
    itor->firstInstance = 0U;
  }
  indirect_draws_buff_.Map(vulkan()->device(), &mapped_memory,
                           SCAST_U32(indirect_draw_cmds.size()) *
                               SCAST_U32(sizeof(VkDrawIndexedIndirectCommand)));
  uint32_t sizedrawinx = sizeof(VkDrawIndexedIndirectCommand);
  memcpy(mapped_memory, SCAST_CVOIDPTR(indirect_draw_cmds.data()),
         SCAST_U32(indirect_draw_cmds.size()) *
             SCAST_U32(sizeof(VkDrawIndexedIndirectCommand)));
  indirect_draws_buff_.Unmap(vulkan()->device());
}

void Model::Shutdown(const VulkanDevice &device) {
  index_buffer_.Shutdown(device);
  for (eastl::vector<VulkanBuffer>::iterator i = vertex_buffers_.begin();
       i != vertex_buffers_.end(); ++i) {
    i->Shutdown(device);
  }
  indirect_draws_buff_.Shutdown(device);
  model_matxs_buff_.Shutdown(device);
  materialIDs_buff_.Shutdown(device);
}

void Model::BindVertexBuffer(VkCommandBuffer cmd_buff) const {
  uint32_t num_elements = SCAST_U32(vertex_buffers_.size());
  eastl::vector<VkDeviceSize> offsets;
  offsets.assign(num_elements, 0U);
  eastl::vector<VkBuffer> buffers(num_elements);
  eastl::vector<VkBuffer>::iterator bi = buffers.begin();
  for (eastl::vector<VulkanBuffer>::const_iterator i = vertex_buffers_.begin();
       i != vertex_buffers_.end(); ++i, ++bi) {
    *bi = i->buffer();
  }

  vkCmdBindVertexBuffers(cmd_buff, 0U, num_elements, buffers.data(),
                         offsets.data());
}

void Model::BindIndexBuffer(VkCommandBuffer cmd_buff) const {
  vkCmdBindIndexBuffer(cmd_buff, index_buffer_.buffer(), 0U,
                       VK_INDEX_TYPE_UINT32);
}

void Model::CreateDescriptorSet(const VulkanDevice &device,
                                VkDescriptorSetLayout heap_set_layout) {
  // Create descriptor set for this heap
  VkDescriptorSetAllocateInfo set_allocate_info =
      tools::inits::DescriptorSetAllocateInfo(desc_pool_, 1U, &heap_set_layout);

  VK_CHECK_RESULT(vkAllocateDescriptorSets(device.device(), &set_allocate_info,
                                           &desc_set_));
}

void Model::WriteDescriptorSet(const VulkanDevice &device) {
  eastl::vector<VkWriteDescriptorSet> write_desc_sets;

  uint32_t counter = 0U;
  eastl::vector<VkDescriptorBufferInfo> elems_buff_infos(
      vertex_buffers_.size());

  for (eastl::vector<VulkanBuffer>::iterator itor = vertex_buffers_.begin();
       itor != vertex_buffers_.end(); ++itor, ++counter) {
    elems_buff_infos[counter] = itor->GetDescriptorBufferInfo();
    uint32_t pos =
        kVertexBuffersBaseBindPos + vtx_setup_.GetElementPosition(counter);
    write_desc_sets.push_back(tools::inits::WriteDescriptorSet(
        desc_set_, pos, 0U, 1U, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, nullptr,
        &elems_buff_infos[counter], nullptr));
  }

  VkDescriptorBufferInfo idx_buff_info =
      index_buffer_.GetDescriptorBufferInfo();
  write_desc_sets.push_back(tools::inits::WriteDescriptorSet(
      desc_set_, kIdxBufferBindPos, 0U, 1U, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
      nullptr, &idx_buff_info, nullptr));

  VkDescriptorBufferInfo model_matxs_buff_info =
      model_matxs_buff_.GetDescriptorBufferInfo();
  write_desc_sets.push_back(tools::inits::WriteDescriptorSet(
      desc_set_, kModelMatxsBufferBindPos, 0U, 1U,
      VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, nullptr, &model_matxs_buff_info,
      nullptr));

  VkDescriptorBufferInfo materialIDs_buff_info =
      materialIDs_buff_.GetDescriptorBufferInfo();
  write_desc_sets.push_back(tools::inits::WriteDescriptorSet(
      desc_set_, kMaterialIDsBufferBindPos, 0U, 1U,
      VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, nullptr, &materialIDs_buff_info,
      nullptr));

  VkDescriptorBufferInfo desc_indirect_draw_buff_info =
      indirect_draws_buff_.GetDescriptorBufferInfo();
  write_desc_sets.push_back(tools::inits::WriteDescriptorSet(
      desc_set_, kIndirectDrawCmdsBindingPos, 0U, 1U,
      VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, nullptr, &desc_indirect_draw_buff_info,
      nullptr));

  vkUpdateDescriptorSets(device.device(), SCAST_U32(write_desc_sets.size()),
                         write_desc_sets.data(), 0U, nullptr);
}

void Model::RenderMeshesByMaterial(VkCommandBuffer cmd_buff,
                                   VkPipelineLayout pipe_layout,
                                   uint32_t desc_set_slot) const {
  // Set descriptor set
  vkCmdBindDescriptorSets(cmd_buff, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          pipe_layout, desc_set_slot, 1U, &desc_set_, 0U,
                          nullptr);

  uint32_t mesh_idx = 0U;
  uint32_t uint32_t_size = SCAST_U32(sizeof(uint32_t));
  for (eastl::vector<Mesh>::const_iterator itor = meshes_.begin();
       itor != meshes_.end(); itor++, mesh_idx++) {
    // Set the mesh ID
    vkCmdPushConstants(cmd_buff, pipe_layout, VK_SHADER_STAGE_VERTEX_BIT, 0U,
                       uint32_t_size, &mesh_idx);

    // Render the mesh
    vkCmdDrawIndexed(cmd_buff, itor->index_count(), 1U, itor->start_index(),
                     itor->vertex_offset(), 0U);
  }

  // typedef std::map<uint32_t, eastl::vector<const Mesh *>>::const_iterator
  // itortp;
  // for (itortp it = meshes_by_material_.begin(); it !=
  // meshes_by_material_.end();
  //     it++) {
  //  const MaterialInstance &mat_inst =
  //    material_manager()->GetMaterialInstance(it->first);
  //
  //  uint32_t meshes_num = SCAST_U32(it->second.size());
  //  uint32_t dynamic_ubo_offset = 0U;
  //  for (uint32_t i = 0U; i < meshes_num; i++) {
  //    dynamic_ubo_offset = it->second[i]->dynamic_ubo_offset();
  //    // Set model mat buffer
  //    vkCmdBindDescriptorSets(
  //      cmd_buff,
  //      VK_PIPELINE_BIND_POINT_GRAPHICS,
  //      pipe_layout,
  //      DescSetLayoutTypes::GPASS_MODELMATXS,
  //      1U,
  //      &model_matx_desc_set_,
  //      1U,
  //      &dynamic_ubo_offset
  //    );
  //
  //    vkCmdDrawIndexed(
  //        cmd_buff,
  //        it->second[i]->index_count(),
  //        1U,
  //        it->second[i]->start_index(),
  //        it->second[i]->vertex_offset(),
  //        0U);
  //  }
  //}
}

void Model::SetModelMatrixForAllMeshes(const glm::mat4 &mat) {
  // std::for_each(
  //    meshes_.begin(),
  //    meshes_.end(),
  //    [&](Mesh &mesh){
  //      mesh.set_model_mat(mat);
  //      void *data = nullptr;
  //      matx_buff_.Map(
  //          vulkan()->device(),
  //          &data);
  //
  //      memcpy(
  //          data,
  //          &mat,
  //          SCAST_U32(sizeof(glm::mat4)));

  //      matx_buff_.Unmap(vulkan()->device());

  //    });
}

void Model::CreateAndWriteDescriptorSets(
    const VulkanDevice &device, VkDescriptorSetLayout heap_set_layout) {
  CreateDescriptorSet(device, heap_set_layout);

  WriteDescriptorSet(device);
}

uint32_t Model::NumMeshes() const { return SCAST_U32(meshes_.size()); }

} // namespace vks
