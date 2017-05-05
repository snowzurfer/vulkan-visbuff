#ifndef VKS_RENDERPASS
#define VKS_RENDERPASS

#include <EASTL/string.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/vector.h>
#include <cstdint>
#include <subpass.h>
#include <vulkan/vulkan.h>

namespace vks {

class VulkanDevice;
class Framebuffer;

class Renderpass {
public:
  Renderpass(const eastl::string &name);
  ~Renderpass();

  uint32_t AddAttachment(VkAttachmentDescriptionFlags flags, VkFormat format,
                         VkSampleCountFlagBits samples,
                         VkAttachmentLoadOp load_op,
                         VkAttachmentStoreOp store_op,
                         VkAttachmentLoadOp stencil_load_op,
                         VkAttachmentStoreOp stencil_store_op,
                         VkImageLayout initial_layout,
                         VkImageLayout final_layout);

  uint32_t AddSubpass(const eastl::string &name,
                      VkPipelineBindPoint bind_point);
  void AddSubpassInputAttachmentRef(uint32_t subpass_id, uint32_t attach_id,
                                    VkImageLayout layout);
  void AddSubpassColourAttachmentRef(uint32_t subpass_id, uint32_t attach_id,
                                     VkImageLayout layout);
  void AddSubpassDepthAttachmentRef(uint32_t subpass_id, uint32_t attach_id,
                                    VkImageLayout layout);
  void AddSubpassPreserveAttachmentRef(uint32_t subpass_id, uint32_t attach_id);

  void AddSubpassDependency(uint32_t src_subpass, uint32_t dst_subpass,
                            VkPipelineStageFlags src_stage_mask,
                            VkPipelineStageFlags dst_stage_mask,
                            VkAccessFlags src_access_mask,
                            VkAccessFlags dst_access_mask,
                            VkDependencyFlags dependency_flags);

  VkRenderPass CreateVulkanRenderpass(const VulkanDevice &device);

  VkRenderPass GetVkRenderpass() const;

  void BeginRenderpass(VkCommandBuffer cmd_buff,
                       VkSubpassContents subpass_contents,
                       Framebuffer *framebuffer, VkRect2D render_area,
                       uint32_t clear_value_count,
                       const VkClearValue *p_clear_values);

  void NextSubpass(VkCommandBuffer cmd_buff,
                   VkSubpassContents subpass_contents);

  void EndRenderpass(VkCommandBuffer cmd_buff);

private:
  eastl::vector<VkAttachmentDescription> attachments_;
  eastl::vector<VkSubpassDependency> dependencies_;
  eastl::vector<eastl::unique_ptr<Subpass>> subpasses_;
  VkRenderPass vk_renderpass_;
  bool is_vkrenderpass_created_;
  eastl::string name_;
  const VulkanDevice *device_;
  mutable Framebuffer *current_framebuffer_;
  uint32_t current_subpass_;

  void
  GetAllSubpassDescriptions(eastl::vector<VkSubpassDescription> &descs) const;

  void SetFramebufferImageLayout();

}; // class Renderpass

} // namespace vks

#endif
