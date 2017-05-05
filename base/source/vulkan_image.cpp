#include <logger.hpp>
#include <utility>
#include <vulkan_device.h>
#include <vulkan_image.h>
#include <vulkan_tools.h>

namespace vks {

VulkanImage::VulkanImage()
    : image_(VK_NULL_HANDLE), owns_image_(false), memory_(VK_NULL_HANDLE),
      size_(0U), default_view_(VK_NULL_HANDLE), additional_views_(),
      memory_properties_flags_(), layout_(), extent_({0U, 0U, 0U}),
      mip_levels_(1U), array_layers_(1U), format_() {}

void VulkanImage::Init(const VulkanDevice &device,
                       const VulkanImageInitInfo &info) {
  memory_properties_flags_ = info.memory_properties_flags;
  layout_ = info.create_info.initialLayout;
  extent_ = info.create_info.extent;
  mip_levels_ = info.create_info.mipLevels;
  format_ = info.create_info.format;
  array_layers_ = info.create_info.arrayLayers;

  // Create the image handle
  VK_CHECK_RESULT(
      vkCreateImage(device.device(), &info.create_info, nullptr, &image_));

  // Check what the memory requirements for the image are
  VkMemoryRequirements memory_requirements;
  vkGetImageMemoryRequirements(device.device(), image_, &memory_requirements);

  // Allocate memory for the image
  VkMemoryAllocateInfo mem_alloc_info = {
      VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, nullptr, memory_requirements.size,
      device.GetMemoryType(memory_requirements.memoryTypeBits,
                           memory_properties_flags_)};

  VK_CHECK_RESULT(
      vkAllocateMemory(device.device(), &mem_alloc_info, nullptr, &memory_));

  // Assign the memory to the image
  VK_CHECK_RESULT(vkBindImageMemory(device.device(), image_, memory_, 0U));

  if (info.create_view == CreateView::YES) {
    // Create an image view for the texture
    VkImageViewCreateInfo img_view_create_info =
        tools::inits::ImageViewCreateInfo(
            image_, info.view_type, format_,
            {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
             VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
            {VK_IMAGE_ASPECT_COLOR_BIT, 0U, mip_levels_, 0U, array_layers_});

    if (info.create_info.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
      img_view_create_info.subresourceRange = {VK_IMAGE_ASPECT_DEPTH_BIT |
                                                   VK_IMAGE_ASPECT_STENCIL_BIT,
                                               0U, mip_levels_, 0U, 1U};
    }

    VK_CHECK_RESULT(vkCreateImageView(device.device(), &img_view_create_info,
                                      nullptr, &default_view_));
  }

  size_ = memory_requirements.size;
  owns_image_ = true;
}

void VulkanImage::Init(const VulkanDevice &device,
                       const VulkanImageAcquireInitInfo &info) {
  image_ = info.image;
  format_ = info.format;
  extent_ = info.extents;

  if (info.create_view == CreateView::YES) {
    // Create an image view for the texture
    VkImageViewCreateInfo img_view_create_info =
        tools::inits::ImageViewCreateInfo(
            image_, info.view_type, format_,
            {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
             VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
            {VK_IMAGE_ASPECT_COLOR_BIT, 0U, mip_levels_, 0U, 1U});

    if (info.image_usages & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
      img_view_create_info.subresourceRange = {VK_IMAGE_ASPECT_DEPTH_BIT |
                                                   VK_IMAGE_ASPECT_STENCIL_BIT,
                                               0U, mip_levels_, 0U, 1U};
    }

    VK_CHECK_RESULT(vkCreateImageView(device.device(), &img_view_create_info,
                                      nullptr, &default_view_));
  }
}

void VulkanImage::Shutdown(const VulkanDevice &device) {
  if (default_view_ != VK_NULL_HANDLE) {
    vkDestroyImageView(device.device(), default_view_, nullptr);
    default_view_ = VK_NULL_HANDLE;
  }
  eastl::vector<VkImageView>::iterator itor;
  for (itor = additional_views_.begin(); itor != additional_views_.end();
       ++itor) {
    vkDestroyImageView(device.device(), *itor, nullptr);
  }
  additional_views_.clear();

  if (image_ != VK_NULL_HANDLE && owns_image_) {
    vkDestroyImage(device.device(), image_, nullptr);
    image_ = VK_NULL_HANDLE;
  }
  if (memory_ != VK_NULL_HANDLE && owns_image_) {
    vkFreeMemory(device.device(), memory_, nullptr);
    memory_ = VK_NULL_HANDLE;
  }
}

VkDescriptorImageInfo
VulkanImage::GetDescriptorImageInfo(VkSampler sampler) const {
  VkDescriptorImageInfo desc;
  desc.imageView = default_view_;
  desc.imageLayout = layout_;
  desc.sampler = sampler;

  return desc;
}

VkDescriptorImageInfo VulkanImage::GetDescriptorImageInfo() const {
  VkDescriptorImageInfo desc;
  desc.imageView = default_view_;
  desc.imageLayout = layout_;
  desc.sampler = VK_NULL_HANDLE;

  return desc;
}

VkImageView *VulkanImage::CreateAdditionalImageView(
    const VulkanDevice &device,
    const VkImageViewCreateInfo &img_view_create_info) const {

  VkImageView img_view = VK_NULL_HANDLE;
  VK_CHECK_RESULT(vkCreateImageView(device.device(), &img_view_create_info,
                                    nullptr, &img_view));

  additional_views_.push_back(img_view);

  return &additional_views_.back();
}

} // namespace vks
