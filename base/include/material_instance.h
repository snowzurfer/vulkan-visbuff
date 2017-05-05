#ifndef VKS_MATERIALINSTANCE
#define VKS_MATERIALINSTANCE

#include <EASTL/array.h>
#include <EASTL/string.h>
#include <EASTL/vector.h>
#include <cstdint>
#include <material_constants.h>
#include <material_texture_type.h>
#include <renderer_type.h>
#include <vulkan_tools.h>

namespace vks {

class Material;
class VulkanDevice;
class VulkanTexture;
class VulkanBuffer;

extern const uint32_t kMapsBaseBindingPos;

struct MaterialBuilderTexture {
  eastl::string name;
  MatTextureType type;
}; // struct MaterialBuilderTexture

class MaterialInstanceBuilder {
public:
  MaterialInstanceBuilder(const eastl::string &inst_name,
                          const eastl::string &mats_directory,
                          const VkSampler aniso_sampler);

  void AddTexture(const MaterialBuilderTexture &texture_info);
  void AddConstants(const MaterialConstants &consts);

  const eastl::vector<MaterialConstants> &consts() const { return consts_; }
  const eastl::vector<MaterialBuilderTexture> &textures() const {
    return textures_;
  }

  const eastl::string &mats_directory() const { return mats_directory_; }
  const eastl::string &inst_name() const { return inst_name_; }

  VkSampler aniso_sampler() const { return aniso_sampler_; };

private:
  eastl::string inst_name_;
  eastl::string mats_directory_;
  eastl::vector<MaterialConstants> consts_;
  eastl::vector<MaterialBuilderTexture> textures_;
  VkSampler aniso_sampler_;

}; // class MaterialInstanceBuilder

class MaterialInstance {
public:
  MaterialInstance();

  void Init(const VulkanDevice &device, const MaterialInstanceBuilder &builder);
  void Shutdown(const VulkanDevice &device);

  const MaterialConstants &consts() const { return consts_; }
  const eastl::array<VulkanTexture *, SCAST_U32(MatTextureType::size)>
  textures() const {
    return textures_;
  }

private:
  eastl::string name_;
  MaterialConstants consts_;
  eastl::array<VulkanTexture *, SCAST_U32(MatTextureType::size)> textures_;
  const Material *material_;
  VkDescriptorSet maps_desc_set_;
  VkSampler aniso_sampler_;

}; // class MaterialInstance

} // namespace vks

#endif
