#ifndef VKS_FRAMEBUFFER
#define VKS_FRAMEBUFFER

#include <EASTL/string.h>
#include <EASTL/vector.h>
#include <vulkan/vulkan.h>

namespace vks {

class VulkanTexture;
class Renderpass;
class VulkanDevice;

class Framebuffer {
public:
  Framebuffer(const eastl::string &name, uint32_t width, uint32_t height,
              uint32_t layers, const Renderpass *renderpass);

  ~Framebuffer();

  void AddAttachment(VulkanTexture *image);

  void CreateVulkanFramebuffer(const VulkanDevice &device);

  void SetAttachmentLayout(uint32_t idx, VkImageLayout layout);

  VkFramebuffer vk_frmbuff() const { return vk_frmbuff_; }

private:
  eastl::string name_;
  VkFramebuffer vk_frmbuff_;
  bool is_vkfrmbuff_created_;
  eastl::vector<VulkanTexture *> attachments_;
  eastl::vector<VkImageView> attachment_views_;
  const Renderpass *renderpass_;
  const VulkanDevice *device_;
  uint32_t width_;
  uint32_t height_;
  uint32_t layers_;

}; // class Framebuffer

} // namespace vks

#endif
