#ifndef VKS_VULKANTEXTURE
#define VKS_VULKANTEXTURE

#include <EASTL/string.h>
#include <EASTL/unique_ptr.h>
#include <cstdint>
#include <vulkan/vulkan.h>
#include <vulkan_image.h>

namespace vks {

enum class CreateSampler : uint8_t { YES = 0U, NO }; // enum class CreateSampler

struct VulkanTextureInitInfo {
  eastl::unique_ptr<VulkanImage> image;
  CreateSampler create_sampler;
  VkSamplerCreateInfo sampler_create_info;
  eastl::string name;
  VkSampler sampler;
}; // struct VulkanTextureInitInfo

class VulkanDevice;

class VulkanTexture {
public:
  VulkanTexture();

  void Init(const VulkanDevice &device, VulkanTextureInitInfo &init_info);
  void Shutdown(const VulkanDevice &device);

  void set_sampler(const VkSampler sampler) { sampler_ = sampler; };
  VkSampler sampler() const { return sampler_; };

  // Get the DescriptorImageInfo relative to this texture so that
  // it can be used in descriptor sets updates
  /**
   * @brief Get the descriptorimageinfo relative to this texture
   *
   * @return A VkDescriptorImageInfo relative to this texture
   */
  VkDescriptorImageInfo GetDescriptorImageInfo() const;

  const VulkanImage *image() const { return image_.get(); };
  VulkanImage *image() { return image_.get(); };

private:
  eastl::string name_;
  eastl::unique_ptr<VulkanImage> image_;
  VkSampler sampler_;

}; // class VulkanTexture

} // namespace vks

#endif
