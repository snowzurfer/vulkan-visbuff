#ifndef VKS_MATERIALMANAGER
#define VKS_MATERIALMANAGER

#include <EASTL/hash_map.h>
#include <EASTL/string.h>
#include <EASTL/vector.h>
#include <material.h>
#include <material_instance.h>
#include <unordered_map>

namespace vks {

class VulkanDevice;
class VulkanBuffer;

class MaterialManager {
public:
  MaterialManager();

  Material *CreateMaterial(const VulkanDevice &device,
                           eastl::unique_ptr<MaterialBuilder> builder);
  void RegisterMaterialName(const eastl::string &name);
  MaterialInstance *
  CreateMaterialInstance(const VulkanDevice &device,
                         const MaterialInstanceBuilder &builder);

  const MaterialInstance &GetMaterialInstance(const eastl::string &name) const;
  const MaterialInstance &GetMaterialInstance(uint32_t index) const;
  const Material *GetMaterial(const eastl::string &name) const;
  uint32_t GetMaterialInstancesCount() const;

  void GetMaterialConstantsBuffer(const VulkanDevice &device,
                                  VulkanBuffer &buffer) const;

  eastl::vector<MaterialConstants> GetMaterialConstants() const;

  void ReloadAllShaders(const VulkanDevice &device);

  /**
   * @brief Get all the descriptor infos of a given type of texture for all the
   *        existing textures.
   *
   * @param texture_type Type of texture (eg. diffuse, specular, etc)
   * @param descs Vector where the descriptors are returned
   */
  void
  GetDescriptorImageInfosByType(const MatTextureType texture_type,
                                eastl::vector<VkDescriptorImageInfo> &descs);

  void Shutdown(const VulkanDevice &device);

private:
  // List of all materials
  typedef eastl::hash_map<eastl::string, eastl::unique_ptr<Material>>
      NameMaterialMap;
  NameMaterialMap materials_map_;

  eastl::hash_map<eastl::string, bool> registered_names_;

  // List of all material instances
  typedef eastl::hash_map<eastl::string, MaterialInstance *>
      NameMaterialInstMap;
  NameMaterialInstMap material_instances_map_;

  eastl::vector<MaterialInstance> material_instances_;

}; // class MaterialManager

} // namespace vks

#endif
