#include <framebuffer.h>
#include <logger.hpp>
#include <renderpass.h>
#include <vulkan_device.h>
#include <vulkan_tools.h>

namespace vks {

Renderpass::Renderpass(const eastl::string &name)
    : attachments_(), dependencies_(), subpasses_(),
      vk_renderpass_(VK_NULL_HANDLE), is_vkrenderpass_created_(false),
      name_(name), device_(nullptr), current_framebuffer_(nullptr),
      current_subpass_(0U) {
  LOG("Create renderpass " << name_);
}

Renderpass::~Renderpass() {
  if (is_vkrenderpass_created_) {
    vkDestroyRenderPass(device_->device(), vk_renderpass_, nullptr);
    vk_renderpass_ = VK_NULL_HANDLE;
  }

  LOG("Destroy renderpass " << name_);
}

uint32_t Renderpass::AddAttachment(
    VkAttachmentDescriptionFlags flags, VkFormat format,
    VkSampleCountFlagBits samples, VkAttachmentLoadOp load_op,
    VkAttachmentStoreOp store_op, VkAttachmentLoadOp stencil_load_op,
    VkAttachmentStoreOp stencil_store_op, VkImageLayout initial_layout,
    VkImageLayout final_layout) {
  attachments_.push_back(tools::inits::AttachmentDescription(
      flags, format, samples, load_op, store_op, stencil_load_op,
      stencil_store_op, initial_layout, final_layout));

  return (SCAST_U32(attachments_.size()) - 1U);
}

uint32_t Renderpass::AddSubpass(const eastl::string &name,
                                VkPipelineBindPoint bind_point) {
  subpasses_.push_back(eastl::make_unique<Subpass>(name, bind_point));

  return (SCAST_U32(subpasses_.size()) - 1U);
}

void Renderpass::AddSubpassColourAttachmentRef(uint32_t subpass_id,
                                               uint32_t attach_id,
                                               VkImageLayout layout) {
  VKS_ASSERT(subpass_id <= (SCAST_U32(subpasses_.size()) - 1U),
             "Subpass with id " << subpass_id << " does not exist!");
  VKS_ASSERT(attach_id <= (SCAST_U32(attachments_.size()) - 1U),
             "Attachment with id " << attach_id << " does not exist!");

  subpasses_[subpass_id]->AddColourAttachmentRef(attach_id, layout);
}

void Renderpass::AddSubpassDepthAttachmentRef(uint32_t subpass_id,
                                              uint32_t attach_id,
                                              VkImageLayout layout) {
  VKS_ASSERT(subpass_id <= (SCAST_U32(subpasses_.size()) - 1U),
             "Subpass with id " << subpass_id << " does not exist!");
  VKS_ASSERT(attach_id <= (SCAST_U32(attachments_.size()) - 1U),
             "Attachment with id " << attach_id << " does not exist!");

  subpasses_[subpass_id]->AddDepthAttachmentRef(attach_id, layout);
}

void Renderpass::AddSubpassInputAttachmentRef(uint32_t subpass_id,
                                              uint32_t attach_id,
                                              VkImageLayout layout) {
  VKS_ASSERT(subpass_id <= (SCAST_U32(subpasses_.size()) - 1U),
             "Subpass with id " << subpass_id << " does not exist!");
  VKS_ASSERT(attach_id <= (SCAST_U32(attachments_.size()) - 1U),
             "Attachment with id " << attach_id << " does not exist!");

  subpasses_[subpass_id]->AddInputAttachmentRef(attach_id, layout);
}

void Renderpass::AddSubpassPreserveAttachmentRef(uint32_t subpass_id,
                                                 uint32_t attach_id) {
  VKS_ASSERT(subpass_id <= (SCAST_U32(subpasses_.size()) - 1U),
             "Subpass with id " << subpass_id << " does not exist!");
  VKS_ASSERT(attach_id <= (SCAST_U32(attachments_.size()) - 1U),
             "Attachment with id " << attach_id << " does not exist!");

  subpasses_[subpass_id]->AddPreserveAttachmentRef(attach_id);
}

void Renderpass::AddSubpassDependency(uint32_t src_subpass,
                                      uint32_t dst_subpass,
                                      VkPipelineStageFlags src_stage_mask,
                                      VkPipelineStageFlags dst_stage_mask,
                                      VkAccessFlags src_access_mask,
                                      VkAccessFlags dst_access_mask,
                                      VkDependencyFlags dependency_flags) {
  VKS_ASSERT(src_subpass <= (SCAST_U32(subpasses_.size()) - 1U) ||
                 src_subpass == VK_SUBPASS_EXTERNAL,
             "Src subpass with id " << src_subpass << " does not exist!");
  VKS_ASSERT(dst_subpass <= (SCAST_U32(subpasses_.size()) - 1U) ||
                 dst_subpass == VK_SUBPASS_EXTERNAL,
             "Dst subpass with id " << dst_subpass << " does not exist!");

  dependencies_.push_back(tools::inits::SubpassDependency(
      src_subpass, dst_subpass, src_stage_mask, dst_stage_mask, src_access_mask,
      dst_access_mask, dependency_flags));
}

void Renderpass::GetAllSubpassDescriptions(
    eastl::vector<VkSubpassDescription> &descs) const {
  for (eastl::vector<eastl::unique_ptr<Subpass>>::const_iterator itor =
           subpasses_.begin();
       itor != subpasses_.end(); ++itor) {
    descs.push_back((*itor)->GetDescription());
  }
}

VkRenderPass Renderpass::CreateVulkanRenderpass(const VulkanDevice &device) {
  eastl::vector<VkSubpassDescription> subpass_descs;
  GetAllSubpassDescriptions(subpass_descs);

  VkRenderPassCreateInfo render_pass_create_info =
      tools::inits::RenderPassCreateInfo(
          SCAST_U32(attachments_.size()), attachments_.data(),
          SCAST_U32(subpass_descs.size()), subpass_descs.data(),
          SCAST_U32(dependencies_.size()), dependencies_.data());

  VK_CHECK_RESULT(vkCreateRenderPass(device.device(), &render_pass_create_info,
                                     nullptr, &vk_renderpass_));

  device_ = &device;
  is_vkrenderpass_created_ = true;

  // attachments_.clear();
  // dependencies_.clear();
  // subpasses_.clear();

  LOG("Successfully created vulkan renderpass for renderpass " << name_);

  return vk_renderpass_;
}

VkRenderPass Renderpass::GetVkRenderpass() const {
  VKS_ASSERT(is_vkrenderpass_created_ == true, "The vulkan renderpass has not\
      been created yet!");

  return vk_renderpass_;
}

void Renderpass::BeginRenderpass(VkCommandBuffer cmd_buff,
                                 VkSubpassContents subpass_contents,
                                 Framebuffer *framebuffer, VkRect2D render_area,
                                 uint32_t clear_value_count,
                                 const VkClearValue *p_clear_values) {
  VkRenderPassBeginInfo render_pass_begin_info =
      tools::inits::RenderPassBeginInfo();
  render_pass_begin_info.renderPass = GetVkRenderpass();
  render_pass_begin_info.framebuffer = framebuffer->vk_frmbuff();
  render_pass_begin_info.renderArea = render_area;
  render_pass_begin_info.clearValueCount = clear_value_count;
  render_pass_begin_info.pClearValues = p_clear_values;

  vkCmdBeginRenderPass(cmd_buff, &render_pass_begin_info, subpass_contents);

  current_framebuffer_ = framebuffer;

  SetFramebufferImageLayout();
}

void Renderpass::NextSubpass(VkCommandBuffer cmd_buff,
                             VkSubpassContents subpass_contents) {

  // Light shading pass
  vkCmdNextSubpass(cmd_buff, subpass_contents);

  ++current_subpass_;
  SetFramebufferImageLayout();
}

void Renderpass::EndRenderpass(VkCommandBuffer cmd_buff) {
  vkCmdEndRenderPass(cmd_buff);
  current_subpass_ = 0U;
  current_framebuffer_ = nullptr;
}

void Renderpass::SetFramebufferImageLayout() {
  VKS_ASSERT(current_framebuffer_ != nullptr, "Current framebuffer not set!");

  subpasses_[current_subpass_]->SetFramebufferImagesLayout(
      current_framebuffer_);
}

} // namespace vks
