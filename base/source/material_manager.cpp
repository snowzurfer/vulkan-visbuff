#include <logger.hpp>
#include <material_constants.h>
#include <material_manager.h>
#include <utility>
#include <vulkan_buffer.h>
#include <vulkan_device.h>
#include <vulkan_texture.h>
#include <vulkan_tools.h>
#include <vulkan_tools.h>

namespace vks {

MaterialManager::MaterialManager()
    : materials_map_(), material_instances_map_(), material_instances_() {}

void MaterialManager::Shutdown(const VulkanDevice &device) {
  uint32_t mat_inst_count = SCAST_U32(material_instances_.size());
  for (uint32_t i = 0U; i < mat_inst_count; i++) {
    material_instances_[i].Shutdown(device);
  }
  material_instances_.clear();
  material_instances_map_.clear();

  NameMaterialMap::iterator iter;
  for (iter = materials_map_.begin(); iter != materials_map_.end(); iter++) {
    iter->second->Shutdown(device);
  }
}

void MaterialManager::RegisterMaterialName(const eastl::string &name) {
  registered_names_[name] = true;
}

Material *
MaterialManager::CreateMaterial(const VulkanDevice &device,
                                eastl::unique_ptr<MaterialBuilder> builder) {
  if (SCAST_U32(materials_map_.count(builder->mat_name()) != 0U)) {
    LOG("Material " << builder->mat_name() << "already exists. \
Returning existing one!");
    return materials_map_[builder->mat_name()].get();
  }

  // Create new material
  materials_map_[builder->mat_name()] = eastl::make_unique<Material>();
  Material *material = materials_map_[builder->mat_name()].get();

  // Initialise it
  material->Init(builder->mat_name());

  // Initialise its pipeline
  material->InitPipeline(device, eastl::move(builder));

  LOG("Added Material " << material->name() << ".");
  return material;
}

MaterialInstance *MaterialManager::CreateMaterialInstance(
    const VulkanDevice &device, const MaterialInstanceBuilder &builder) {
  NameMaterialInstMap::iterator instance_found =
      material_instances_map_.find(builder.inst_name());
  if (instance_found != material_instances_map_.end()) {
    LOG("Material instance already exists");
    return instance_found->second;
  }

  MaterialInstance instance;
  instance.Init(device, builder);
  material_instances_.push_back(instance);
  material_instances_map_[builder.inst_name()] = &material_instances_.back();
  return &material_instances_.back();
}

const MaterialInstance &
MaterialManager::GetMaterialInstance(const eastl::string &name) const {
  return *(material_instances_map_.find(name)->second);
}

const MaterialInstance &
MaterialManager::GetMaterialInstance(uint32_t index) const {
  return material_instances_[index];
}

const Material *MaterialManager::GetMaterial(const eastl::string &name) const {
  NameMaterialMap::const_iterator itor = materials_map_.find(name);
  if (itor != materials_map_.end()) {
    return itor->second.get();
  } else {
    return nullptr;
  }
}

void MaterialManager::GetMaterialConstantsBuffer(const VulkanDevice &device,
                                                 VulkanBuffer &buffer) const {
  uint32_t num_mat_instances = GetMaterialInstancesCount();
  uint32_t mat_constant_size = SCAST_U32(sizeof(MaterialConstants));

  // Create a large enough buffer
  VulkanBufferInitInfo init_info;
  init_info.size = num_mat_instances * mat_constant_size;
  init_info.memory_property_flags = /*VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |*/
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
  init_info.buffer_usage_flags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

  buffer.Init(device, init_info);

  // Upload the material constants data to it
  for (uint32_t i = 0U; i < num_mat_instances; i++) {
    void *data = nullptr;
    buffer.Map(device, &data, mat_constant_size, i * mat_constant_size);
    MaterialConstants *mat_const_ptr = static_cast<MaterialConstants *>(data);
    *mat_const_ptr = material_instances_[i].consts();
    buffer.Unmap(device);
  }
}

uint32_t MaterialManager::GetMaterialInstancesCount() const {
  return SCAST_U32(material_instances_.size());
}

void MaterialManager::GetDescriptorImageInfosByType(
    const MatTextureType texture_type,
    eastl::vector<VkDescriptorImageInfo> &descs) {
  uint32_t num_mat_instances = GetMaterialInstancesCount();

  // For all the material instances
  for (uint32_t i = 0U; i < num_mat_instances; i++) {
    // Read their texture of given type
    VulkanTexture &texture =
        *material_instances_[i].textures()[tools::ToUnderlying(texture_type)];

    // Save it in the list
    descs.push_back(texture.GetDescriptorImageInfo());
  }
}

eastl::vector<MaterialConstants> MaterialManager::GetMaterialConstants() const {
  eastl::vector<MaterialConstants> constants;

  for (eastl::vector<MaterialInstance>::const_iterator i =
           material_instances_.begin();
       i != material_instances_.end(); i++) {
    constants.push_back(i->consts());
  }

  return constants;
}

void MaterialManager::ReloadAllShaders(const VulkanDevice &device) {
  vkDeviceWaitIdle(device.device());

  for (NameMaterialMap::iterator itor = materials_map_.begin();
       itor != materials_map_.end(); itor++) {
    itor->second->Reload(device);
  }
}

} // namespace vks
