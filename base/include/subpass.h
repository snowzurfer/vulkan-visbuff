#ifndef VKS_SUBPASS
#define VKS_SUBPASS

#include <EASTL/string.h>
#include <EASTL/vector.h>
#include <cstdint>
#include <vulkan/vulkan.h>

namespace vks {

class Framebuffer;

class Subpass {
public:
  Subpass(const eastl::string &name, VkPipelineBindPoint bind_point);
  ~Subpass();

  void AddColourAttachmentRef(uint32_t attach_id, VkImageLayout layout);
  void AddInputAttachmentRef(uint32_t attach_id, VkImageLayout layout);
  void AddDepthAttachmentRef(uint32_t attach_id, VkImageLayout layout);
  void AddPreserveAttachmentRef(uint32_t attach_id);

  VkSubpassDescription GetDescription() const;

  void SetFramebufferImagesLayout(Framebuffer *framebuffer);

private:
  eastl::vector<VkAttachmentReference> col_attachment_refs_;
  eastl::vector<VkAttachmentReference> input_attachment_refs_;
  eastl::vector<uint32_t> preserve_attachment_refs_;
  VkAttachmentReference depth_attachment_ref_;
  bool has_depth_attachment_;
  VkPipelineBindPoint bind_point_;
  eastl::string name_;

}; // class Subpass

} // namespace vks

#endif
