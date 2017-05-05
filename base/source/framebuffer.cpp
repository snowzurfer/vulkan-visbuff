#include <framebuffer.h>
#include <logger.hpp>
#include <renderpass.h>
#include <vulkan_device.h>
#include <vulkan_texture.h>
#include <vulkan_tools.h>

namespace vks {

Framebuffer::Framebuffer(const eastl::string &name, uint32_t width,
                         uint32_t height, uint32_t layers,
                         const Renderpass *renderpass)
    : name_(name), vk_frmbuff_(VK_NULL_HANDLE), is_vkfrmbuff_created_(false),
      attachments_(), renderpass_(renderpass), device_(nullptr), width_(width),
      height_(height), layers_(layers) {
  LOG("Create framebuffer " << name_);
}

Framebuffer::~Framebuffer() {
  if (is_vkfrmbuff_created_) {
    vkDeviceWaitIdle(device_->device());
    vkDestroyFramebuffer(device_->device(), vk_frmbuff_, nullptr);
    vk_frmbuff_ = VK_NULL_HANDLE;
  }

  LOG("Destroy framebuffer " << name_);
}

void Framebuffer::AddAttachment(VulkanTexture *image) {
  attachments_.push_back(image);
  attachment_views_.push_back(image->image()->view());
}

void Framebuffer::CreateVulkanFramebuffer(const VulkanDevice &device) {
  VKS_ASSERT(renderpass_ != nullptr, "Renderpass not set!");

  VkFramebufferCreateInfo framebuffer_create_info =
      tools::inits::FramebufferCreateInfo(
          renderpass_->GetVkRenderpass(), SCAST_U32(attachment_views_.size()),
          attachment_views_.data(), width_, height_, layers_);

  VK_CHECK_RESULT(vkCreateFramebuffer(device.device(), &framebuffer_create_info,
                                      nullptr, &vk_frmbuff_));

  device_ = &device;
  is_vkfrmbuff_created_ = true;

  LOG("Successfully created vulkan framebuffer for framebuffer " << name_);
}

void Framebuffer::SetAttachmentLayout(uint32_t idx, VkImageLayout layout) {
  attachments_[idx]->image()->set_layout(layout);
}

} // namespace vks
