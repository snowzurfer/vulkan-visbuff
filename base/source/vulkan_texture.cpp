#include <EASTL/utility.h>
#include <logger.hpp>
#include <utility>
#include <vulkan_device.h>
#include <vulkan_texture.h>
#include <vulkan_tools.h>

namespace vks {

VulkanTexture::VulkanTexture() : image_(), sampler_(VK_NULL_HANDLE) {}

void VulkanTexture::Init(const VulkanDevice &device,
                         VulkanTextureInitInfo &init_info) {
  image_ = eastl::move(init_info.image);

  if (init_info.create_sampler == CreateSampler::YES) {
    VK_CHECK_RESULT(vkCreateSampler(
        device.device(), &init_info.sampler_create_info, nullptr, &sampler_));
  } else {
    sampler_ = init_info.sampler;
  }

  name_ = init_info.name;

  LOG("Initialised texture " + name_ + ".");
}

void VulkanTexture::Shutdown(const VulkanDevice &device) {
  image_->Shutdown(device);
  LOG("Shutdown tex " << name_);
}

VkDescriptorImageInfo VulkanTexture::GetDescriptorImageInfo() const {
  VkDescriptorImageInfo info;
  info.imageView = image_->view();
  info.sampler = sampler_;
  info.imageLayout = image_->layout();

  return info;
}

} // namespace vks
