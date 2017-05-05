#include <framebuffer.h>
#include <logger.hpp>
#include <subpass.h>
#include <vulkan_tools.h>

namespace vks {

Subpass::Subpass(const eastl::string &name, VkPipelineBindPoint bind_point)
    : col_attachment_refs_(), input_attachment_refs_(),
      preserve_attachment_refs_(), depth_attachment_ref_(),
      has_depth_attachment_(false), bind_point_(bind_point), name_(name) {
  LOG("Created supass " << name_);
}

Subpass::~Subpass() { LOG("Dtor subpass " << name_); }

void Subpass::AddColourAttachmentRef(uint32_t attach_id, VkImageLayout layout) {
  col_attachment_refs_.push_back({attach_id, layout});
}

void Subpass::AddInputAttachmentRef(uint32_t attach_id, VkImageLayout layout) {
  input_attachment_refs_.push_back({attach_id, layout});
}

void Subpass::AddPreserveAttachmentRef(uint32_t attach_id) {
  preserve_attachment_refs_.push_back(attach_id);
}

void Subpass::AddDepthAttachmentRef(uint32_t attach_id, VkImageLayout layout) {
  depth_attachment_ref_ = {attach_id, layout};
  has_depth_attachment_ = true;
}

VkSubpassDescription Subpass::GetDescription() const {
  if (has_depth_attachment_) {
    return tools::inits::SubpassDescription(
        bind_point_, SCAST_U32(input_attachment_refs_.size()),
        input_attachment_refs_.data(), SCAST_U32(col_attachment_refs_.size()),
        col_attachment_refs_.data(), nullptr, &depth_attachment_ref_,
        SCAST_U32(preserve_attachment_refs_.size()),
        preserve_attachment_refs_.data());
  } else {
    return tools::inits::SubpassDescription(
        bind_point_, SCAST_U32(input_attachment_refs_.size()),
        input_attachment_refs_.data(), SCAST_U32(col_attachment_refs_.size()),
        col_attachment_refs_.data(), nullptr, nullptr,
        SCAST_U32(preserve_attachment_refs_.size()),
        preserve_attachment_refs_.data());
  }
}

void Subpass::SetFramebufferImagesLayout(Framebuffer *framebuffer) {
  for (eastl::vector<VkAttachmentReference>::iterator i =
           col_attachment_refs_.begin();
       i != col_attachment_refs_.end(); ++i) {
    framebuffer->SetAttachmentLayout(i->attachment, i->layout);
  }

  if (has_depth_attachment_) {
    framebuffer->SetAttachmentLayout(depth_attachment_ref_.attachment,
                                     depth_attachment_ref_.layout);
  }
}

} // namespace vks
