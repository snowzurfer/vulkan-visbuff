#include <base_system.h>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <logger.hpp>
#include <string>
#include <vulkan_device.h>
#include <vulkan_image.h>
#include <vulkan_tools.h>

namespace vks {
namespace tools {

// void Exit(const std::string &msg) {
// ELOG_ERR(msg);
// ELOG_FILE_ERR(msg);
// exit(EXIT_FAILURE);
//}

std::string VkResultToString(VkResult error_code) {
  switch (error_code) {
#define STR_ERR(r)                                                             \
  case VK_##r:                                                                 \
    return #r
    STR_ERR(NOT_READY);
    STR_ERR(TIMEOUT);
    STR_ERR(EVENT_SET);
    STR_ERR(EVENT_RESET);
    STR_ERR(INCOMPLETE);
    STR_ERR(ERROR_OUT_OF_HOST_MEMORY);
    STR_ERR(ERROR_OUT_OF_DEVICE_MEMORY);
    STR_ERR(ERROR_INITIALIZATION_FAILED);
    STR_ERR(ERROR_DEVICE_LOST);
    STR_ERR(ERROR_MEMORY_MAP_FAILED);
    STR_ERR(ERROR_LAYER_NOT_PRESENT);
    STR_ERR(ERROR_EXTENSION_NOT_PRESENT);
    STR_ERR(ERROR_FEATURE_NOT_PRESENT);
    STR_ERR(ERROR_INCOMPATIBLE_DRIVER);
    STR_ERR(ERROR_TOO_MANY_OBJECTS);
    STR_ERR(ERROR_FORMAT_NOT_SUPPORTED);
    STR_ERR(ERROR_SURFACE_LOST_KHR);
    STR_ERR(ERROR_NATIVE_WINDOW_IN_USE_KHR);
    STR_ERR(SUBOPTIMAL_KHR);
    STR_ERR(ERROR_OUT_OF_DATE_KHR);
    STR_ERR(ERROR_INCOMPATIBLE_DISPLAY_KHR);
    STR_ERR(ERROR_VALIDATION_FAILED_EXT);
    STR_ERR(ERROR_INVALID_SHADER_NV);
#undef STR_ERR
  default:
    return "UNKNOWN_ERROR";
  }
}

bool GetSupportedDepthFormat(VkPhysicalDevice physical_device,
                             VkFormat &depth_format) {
  // Since all depth formats may be optional, we need to find a suitable depth
  // format to use.
  // Start with the highest precision packed format
  std::vector<VkFormat> depth_formats = {
      VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT,
      VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D16_UNORM_S8_UINT,
      VK_FORMAT_D16_UNORM};

  uint32_t depth_formats_count = static_cast<uint32_t>(depth_formats.size());
  for (uint32_t i = 0U; i < depth_formats_count; ++i) {
    VkFormatProperties format_properties;
    vkGetPhysicalDeviceFormatProperties(physical_device, depth_formats[i],
                                        &format_properties);
    // Format must support depth stencil attachment for optimal tiling
    if (format_properties.optimalTilingFeatures &
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
      depth_format = depth_formats[i];
      return true;
    }
  }

  return false;
}

uint32_t
GetSwapChainNumImages(const VkSurfaceCapabilitiesKHR &surface_capabilities) {
  uint32_t image_count = surface_capabilities.minImageCount + 1;

  if ((surface_capabilities.maxImageCount > 0) &&
      (image_count > surface_capabilities.maxImageCount)) {
    image_count = surface_capabilities.maxImageCount;
  }

  return image_count;
}

VkSurfaceFormatKHR
GetSwapChainFormat(VkFormat desired_format,
                   const std::vector<VkSurfaceFormatKHR> &surface_formats) {
  // If the list contains only one format and this is of type UNDEFINED then
  // we can use any format we prefer
  if ((surface_formats.size() == 1) &&
      (surface_formats[0].format == VK_FORMAT_UNDEFINED)) {
    return {desired_format, VK_COLORSPACE_SRGB_NONLINEAR_KHR};
  }

  // Check if the list contains the R8 G8 B8 A8 format with non-linear color
  // space
  for (uint32_t i = 0U; i < surface_formats.size(); ++i) {
    if (surface_formats[i].format == desired_format) {
      return surface_formats[i];
    }
  }

  // If not, return the first format from the list
  return surface_formats[0];
}

VkExtent2D
GetSwapChainExtent(const VkSurfaceCapabilitiesKHR &surface_capabilities,
                   const uint32_t width, const uint32_t height) {
  // If the extent value is -1, it indicates we can define the extents
  // ourselves.
  if (surface_capabilities.currentExtent.width == (~0x0u)) {
    VkExtent2D swap_chain_extent = {width, height};

    // However, they still have to be withing the max and min ranges
    if (swap_chain_extent.width < surface_capabilities.minImageExtent.width) {
      swap_chain_extent.width = surface_capabilities.minImageExtent.width;
    }
    if (swap_chain_extent.height < surface_capabilities.minImageExtent.height) {
      swap_chain_extent.height = surface_capabilities.minImageExtent.height;
    }
    if (swap_chain_extent.width > surface_capabilities.maxImageExtent.width) {
      swap_chain_extent.width = surface_capabilities.maxImageExtent.width;
    }
    if (swap_chain_extent.height > surface_capabilities.maxImageExtent.height) {
      swap_chain_extent.height = surface_capabilities.maxImageExtent.height;
    }

    return swap_chain_extent;
  }

  // Most of the cases we define the size of the swap_chain images as equal
  // to current window's size
  return surface_capabilities.currentExtent;
}

bool DoesFileExist(const std::string &name) {
  LOG("Testing file " << name);
  std::ifstream f(name.c_str());
  return f.good();
}

VkImageUsageFlags
GetSwapChainUsageFlags(const VkSurfaceCapabilitiesKHR &surface_capabilities) {
  // The color attachment flag must always be supported
  // We can define other usage flags but we always need to check if they are
  // supported
  if (surface_capabilities.supportedUsageFlags &
      (VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT)) {
    return VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
           VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
  }

  std::cout << "VK_IMAGE_USAGE_TRANSFER_DST image usage is not supported by "
               "the swap chain!"
            << std::endl
            << "Supported swap chain's image usages include:" << std::endl
            << (surface_capabilities.supportedUsageFlags &
                        VK_IMAGE_USAGE_TRANSFER_SRC_BIT
                    ? "    VK_IMAGE_USAGE_TRANSFER_SRC\n"
                    : "")
            << (surface_capabilities.supportedUsageFlags &
                        VK_IMAGE_USAGE_TRANSFER_DST_BIT
                    ? "    VK_IMAGE_USAGE_TRANSFER_DST\n"
                    : "")
            << (surface_capabilities.supportedUsageFlags &
                        VK_IMAGE_USAGE_SAMPLED_BIT
                    ? "    VK_IMAGE_USAGE_SAMPLED\n"
                    : "")
            << (surface_capabilities.supportedUsageFlags &
                        VK_IMAGE_USAGE_STORAGE_BIT
                    ? "    VK_IMAGE_USAGE_STORAGE\n"
                    : "")
            << (surface_capabilities.supportedUsageFlags &
                        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
                    ? "    VK_IMAGE_USAGE_COLOR_ATTACHMENT\n"
                    : "")
            << (surface_capabilities.supportedUsageFlags &
                        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
                    ? "    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT\n"
                    : "")
            << (surface_capabilities.supportedUsageFlags &
                        VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT
                    ? "    VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT\n"
                    : "")
            << (surface_capabilities.supportedUsageFlags &
                        VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT
                    ? "    VK_IMAGE_USAGE_INPUT_ATTACHMENT"
                    : "")
            << std::endl;

  return static_cast<VkImageUsageFlags>(-1);
}

VkSurfaceTransformFlagBitsKHR
GetSwapChainTransform(const VkSurfaceCapabilitiesKHR &surface_capabilities) {
  // Sometimes it's necessary to transform an image before displaying it, such
  // as on a mobile device if it's in landscape/portrait mode.
  // In our case we just use an identity matrix or the first one available
  if (surface_capabilities.supportedTransforms &
      VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
    return VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
  }

  return surface_capabilities.currentTransform;
}

VkPresentModeKHR
GetSwapChainPresentMode(const std::vector<VkPresentModeKHR> &present_modes) {
  // FIFO present mode is always available
  // MAILBOX is the lowest latency V-Sync enabled mode
  for (uint32_t i = 0U; i < present_modes.size(); ++i) {
    if (present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
      return present_modes[i];
    }
  }

  for (uint32_t i = 0U; i < present_modes.size(); ++i) {
    if (present_modes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR) {
      return present_modes[i];
    }
  }

  for (uint32_t i = 0U; i < present_modes.size(); ++i) {
    if (present_modes[i] == VK_PRESENT_MODE_FIFO_KHR) {
      return present_modes[i];
    }
  }

  std::cout << "FIFO present mode is not supported by the swap chain!"
            << std::endl;
  return static_cast<VkPresentModeKHR>(-1);
}

bool DoesPhysicalDeviceSupportExtension(
    const char *extension_name,
    const std::vector<VkExtensionProperties> &available_extensions) {
  for (uint32_t i = 0; i < available_extensions.size(); ++i) {
    if (strcmp(available_extensions[i].extensionName, extension_name) == 0) {
      return true;
    }
  }

  return false;
}

void SetImageLayout(VkCommandBuffer cmd_buff, VulkanImage &image,
                    VkImageLayout old_image_layout,
                    VkImageLayout new_image_layout,
                    VkAccessFlags src_access_flags,
                    VkAccessFlags dst_access_flags,
                    VkImageSubresourceRange subresource_range) {
  // Create an image barrier
  VkImageMemoryBarrier image_barrier = tools::inits::ImageMemoryBarrier();
  image_barrier.oldLayout = old_image_layout;
  image_barrier.newLayout = new_image_layout;
  image_barrier.image = image.image();
  image_barrier.subresourceRange = subresource_range;
  image_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  image_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  image_barrier.srcAccessMask = src_access_flags;
  image_barrier.dstAccessMask = dst_access_flags;

  // Source layouts (old)
  // Source access mask controls actions that have to be finished on the old
  // layout before it will be transitioned to the new layout
  switch (old_image_layout) {
  case VK_IMAGE_LAYOUT_UNDEFINED: {
    // Image layout is undefined (or does not matter)
    // Only valid as initial layout
    // No flags required, listed only for completeness
    image_barrier.srcAccessMask = 0;
    break;
  }
  case VK_IMAGE_LAYOUT_PREINITIALIZED: {
    // Image is preinitialized
    // Only valid as initial layout for linear images,
    // preserves memory contents
    // Make sure host writes have been finished
    image_barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
    break;
  }
  case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL: {
    // Image is a color attachment
    // Make sure any writes to the color buffer have been finished
    image_barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    break;
  }
  case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL: {
    // Image is a depth/stencil attachment
    // Make sure any writes to the depth/stencil buffer have been finished
    image_barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    break;
  }
  case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL: {
    image_barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    break;
  }
  case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL: {
    // Image is a transfer source
    // Make sure any reads from the image have been finished
    image_barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    break;
  }
  case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL: {
    // Image is a transfer destination
    // Make sure any writes to the image have been finished
    image_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    break;
  }
  case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL: {
    // Image is read by a shader
    // Make sure any shader reads from the image have been finished
    image_barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    break;
  }
  case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR: {
    image_barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    break;
  }
  default: { EXIT("Unsupported src layout"); }
  }

  // Target layouts (new)
  switch (new_image_layout) {
  case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL: {
    // Image will be used as a transfer destination
    // Make sure any writes to the image have been finished
    image_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    break;
  }
  case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL: {
    // Image will be used as a transfer source
    // Make sure any reads from and writes to the image have been finished
    image_barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    break;
  }
  case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL: {
    // Image will be used as a color attachment
    image_barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    break;
  }
  case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL: {
    // Image layout will be used as a depth/stencil attachment
    // Make sure any writes to depth/stencil buffer have been finished
    image_barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    break;
  }
  case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL: {
    image_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    break;
  }
  case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL: {
    // Image will be read in a shader (sampler, input attachment)
    // Make sure any writes to the image have been finished
    if (image_barrier.srcAccessMask == 0U) {
      image_barrier.srcAccessMask =
          VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
    }

    image_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    break;
  }
  case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR: {
    image_barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    break;
  }
  default: { EXIT("Unsupported dst layout"); }
  }

  // Put barrier on top
  VkPipelineStageFlags src_stage_flags = VK_PIPELINE_STAGE_TRANSFER_BIT;
  VkPipelineStageFlags dst_stage_flags = VK_PIPELINE_STAGE_TRANSFER_BIT;

  // Put barrier inside setup command buffer
  vkCmdPipelineBarrier(cmd_buff, src_stage_flags, dst_stage_flags, 0U, 0U,
                       nullptr, 0U, nullptr, 1U, &image_barrier);

  image.set_layout(new_image_layout);
}

void SetImageLayout(VkCommandBuffer cmd_buff, vks::VulkanImage &image,
                    VkImageLayout old_image_layout,
                    VkImageLayout new_image_layout,
                    VkImageSubresourceRange subresource_range) {
  // Create an image barrier
  VkImageMemoryBarrier image_barrier = tools::inits::ImageMemoryBarrier();
  image_barrier.oldLayout = image.layout();
  image_barrier.newLayout = new_image_layout;
  image_barrier.image = image.image();
  image_barrier.subresourceRange = subresource_range;
  image_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  image_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

  // Source layouts (old)
  // Source access mask controls actions that have to be finished on the old
  // layout before it will be transitioned to the new layout
  switch (old_image_layout) {
  case VK_IMAGE_LAYOUT_UNDEFINED: {
    // Image layout is undefined (or does not matter)
    // Only valid as initial layout
    // No flags required, listed only for completeness
    image_barrier.srcAccessMask = 0;
    break;
  }
  case VK_IMAGE_LAYOUT_PREINITIALIZED: {
    // Image is preinitialized
    // Only valid as initial layout for linear images,
    // preserves memory contents
    // Make sure host writes have been finished
    image_barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
    break;
  }
  case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL: {
    // Image is a color attachment
    // Make sure any writes to the color buffer have been finished
    image_barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    break;
  }
  case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL: {
    // Image is a depth/stencil attachment
    // Make sure any writes to the depth/stencil buffer have been finished
    image_barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    break;
  }
  case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL: {
    image_barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    break;
  }
  case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL: {
    // Image is a transfer source
    // Make sure any reads from the image have been finished
    image_barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    break;
  }
  case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL: {
    // Image is a transfer destination
    // Make sure any writes to the image have been finished
    image_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    break;
  }
  case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL: {
    // Image is read by a shader
    // Make sure any shader reads from the image have been finished
    image_barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    break;
  }
  case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR: {
    image_barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    break;
  }
  default: { std::cerr << "Unsupported src layout" << std::endl; }
  }

  // Target layouts (new)
  switch (new_image_layout) {
  case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL: {
    // Image will be used as a transfer destination
    // Make sure any writes to the image have been finished
    image_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    break;
  }
  case VK_IMAGE_LAYOUT_GENERAL: {
    // Image will be used as a transfer destination
    // Make sure any writes to the image have been finished
    image_barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    break;
  }
  case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL: {
    // Image will be used as a transfer source
    // Make sure any reads from and writes to the image have been finished
    image_barrier.srcAccessMask |= VK_ACCESS_TRANSFER_READ_BIT;
    image_barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    break;
  }
  case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL: {
    // Image will be used as a color attachment
    image_barrier.srcAccessMask |= VK_ACCESS_TRANSFER_READ_BIT;
    image_barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    break;
  }
  case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL: {
    // Image layout will be used as a depth/stencil attachment
    // Make sure any writes to depth/stencil buffer have been finished
    image_barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    break;
  }
  case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL: {
    image_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    break;
  }
  case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL: {
    // Image will be read in a shader (sampler, input attachment)
    // Make sure any writes to the image have been finished
    if (image_barrier.srcAccessMask == 0U) {
      image_barrier.srcAccessMask =
          VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
    }

    image_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    break;
  }
  case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR: {
    image_barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    break;
  }
  default: { std::cerr << "Unsupported dst layout" << std::endl; }
  }

  // Put barrier on top
  VkPipelineStageFlags src_stage_flags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
  VkPipelineStageFlags dst_stage_flags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

  // Put barrier inside setup command buffer
  vkCmdPipelineBarrier(cmd_buff, src_stage_flags, dst_stage_flags, 0U, 0U,
                       nullptr, 0U, nullptr, 1U, &image_barrier);

  image.set_layout(new_image_layout);
}

void SetImageLayoutAndExecuteBarrier(
    const vks::VulkanDevice &device, VkCommandBuffer cmd_buff,
    VulkanImage &image, VkImageLayout old_image_layout,
    VkImageLayout new_image_layout, VkImageSubresourceRange subresource_range) {

  // Use the separate command buffer for texture loading
  VkCommandBufferBeginInfo cmd_buff_begin_info =
      tools::inits::CommandBufferBeginInfo();
  VK_CHECK_RESULT(vkBeginCommandBuffer(cmd_buff, &cmd_buff_begin_info));

  SetImageLayout(cmd_buff, image, old_image_layout, new_image_layout,
                 subresource_range);

  VK_CHECK_RESULT(vkEndCommandBuffer(cmd_buff));

  // Use a fence to make sure that the copying operations have completed
  // and then submit the command buffer
  VkFenceCreateInfo fence_create_info = tools::inits::FenceCreateInfo();
  VkFence copy_fence = VK_NULL_HANDLE;
  VK_CHECK_RESULT(
      vkCreateFence(device.device(), &fence_create_info, nullptr, &copy_fence));

  VkSubmitInfo submit_info = tools::inits::SubmitInfo();
  submit_info.waitSemaphoreCount = 0U;
  submit_info.pWaitSemaphores = nullptr;
  submit_info.pWaitDstStageMask = nullptr;
  submit_info.commandBufferCount = 1U;
  submit_info.pCommandBuffers = &cmd_buff;
  submit_info.signalSemaphoreCount = 0U;
  submit_info.pSignalSemaphores = nullptr;

  vkQueueSubmit(device.graphics_queue().queue, 1U, &submit_info, copy_fence);

  VK_CHECK_RESULT(
      vkWaitForFences(device.device(), 1U, &copy_fence, VK_TRUE, UINT64_MAX));

  // Cleanup the staging resources
  vkDestroyFence(device.device(), copy_fence, nullptr);
  copy_fence = VK_NULL_HANDLE;
}

void SetImageMemoryBarrier(VkCommandBuffer cmd_buff,
                           const std::vector<VulkanImage *> &images,
                           uint32_t old_family_index, uint32_t new_family_index,
                           VkAccessFlags src_access_mask,
                           VkAccessFlags dst_access_mask,
                           VkImageLayout old_image_layout,
                           VkImageLayout new_image_layout,
                           VkImageSubresourceRange subresource_range,
                           VkPipelineStageFlagBits src_stage_mask,
                           VkPipelineStageFlagBits dst_stage_mask) {
  std::vector<VkImageMemoryBarrier> barriers;

  uint32_t num_images = SCAST_U32(images.size());
  for (uint32_t i = 0U; i < num_images; i++) {
    barriers.push_back(tools::inits::ImageMemoryBarrier(
        src_access_mask, dst_access_mask, old_image_layout, new_image_layout,
        old_family_index, new_family_index, images[i]->image(),
        subresource_range));

    images[i]->set_layout(new_image_layout);
  }

  vkCmdPipelineBarrier(cmd_buff, src_stage_mask, dst_stage_mask,
                       VK_DEPENDENCY_BY_REGION_BIT, 0U, nullptr, 0U, nullptr,
                       num_images, barriers.data());
}

void SetImageMemoryBarrier(VkCommandBuffer cmd_buff, VkImage image,
                           uint32_t old_family_index, uint32_t new_family_index,
                           VkAccessFlags src_access_mask,
                           VkAccessFlags dst_access_mask,
                           VkImageLayout old_image_layout,
                           VkImageLayout new_image_layout,
                           VkImageSubresourceRange subresource_range,
                           VkPipelineStageFlagBits src_stage_mask,
                           VkPipelineStageFlagBits dst_stage_mask) {
  // Create an image barrier
  VkImageMemoryBarrier image_barrier = tools::inits::ImageMemoryBarrier(
      src_access_mask, dst_access_mask, old_image_layout, new_image_layout,
      old_family_index, new_family_index, image, subresource_range);

  vkCmdPipelineBarrier(cmd_buff, src_stage_mask, dst_stage_mask,
                       VK_DEPENDENCY_BY_REGION_BIT, 0U, nullptr, 0U, nullptr,
                       1U, &image_barrier);
}

void SetLayoutAndWrite(uint32_t dst_binding, VkDescriptorType descriptor_type,
                       uint32_t descriptor_count,
                       VkShaderStageFlags stage_flags,
                       uint32_t dst_array_element, VkDescriptorSet dst_set,
                       const VkBufferView *pTexelBufferView,
                       const VkDescriptorImageInfo *pImageInfo,
                       const VkDescriptorBufferInfo *pBufferInfo,
                       std::vector<VkDescriptorSetLayoutBinding> &bindings,
                       std::vector<VkWriteDescriptorSet> &writes) {
  bindings.push_back(inits::DescriptorSetLayoutBinding(
      dst_binding, descriptor_type, descriptor_count, stage_flags, nullptr));
  writes.push_back(inits::WriteDescriptorSet(
      dst_set, dst_binding, dst_array_element, descriptor_count,
      descriptor_type, pImageInfo, pBufferInfo, pTexelBufferView));
}

bool Replace(eastl::string &str, const eastl::string &from,
             const eastl::string &to) {
  size_t start_pos = str.find(from);
  if (start_pos == eastl::string::npos)
    return false;
  str.replace(start_pos, from.length(), to);
  return true;
}

float Lerp(float a, float b, float t) { return a + (t * (b - a)); }

namespace inits {

VkBufferCreateInfo BufferCreateInfo(VkBufferCreateFlags flags) {
  VkBufferCreateInfo structure;
  structure.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  structure.pNext = nullptr;
  structure.flags = flags;

  return structure;
}

VkMemoryAllocateInfo MemoryAllocateInfo() {
  VkMemoryAllocateInfo structure;
  structure.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  structure.pNext = nullptr;

  return structure;
}

VkImageCreateInfo
ImageCreateInfo(VkImageCreateFlags flags, VkImageType imageType,
                VkFormat format, VkExtent3D extent, uint32_t mipLevels,
                uint32_t arrayLayers, VkSampleCountFlagBits samples,
                VkImageTiling tiling, VkImageUsageFlags usage,
                VkSharingMode sharingMode, uint32_t queueFamilyIndexCount,
                const uint32_t *pQueueFamilyIndices,
                VkImageLayout initialLayout) {
  VkImageCreateInfo structure = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                                 nullptr,
                                 flags,
                                 imageType,
                                 format,
                                 extent,
                                 mipLevels,
                                 arrayLayers,
                                 samples,
                                 tiling,
                                 usage,
                                 sharingMode,
                                 queueFamilyIndexCount,
                                 pQueueFamilyIndices,
                                 initialLayout};

  return structure;
}

VkImageViewCreateInfo
ImageViewCreateInfo(VkImage image, VkImageViewType viewType, VkFormat format,
                    VkComponentMapping components,
                    VkImageSubresourceRange subresourceRange) {
  VkImageViewCreateInfo structure = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                                     nullptr,
                                     0U,
                                     image,
                                     viewType,
                                     format,
                                     components,
                                     subresourceRange};

  return structure;
}

VkCommandBufferBeginInfo
CommandBufferBeginInfo(VkCommandBufferUsageFlags flags) {
  VkCommandBufferBeginInfo structure;
  structure.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  structure.pNext = nullptr;
  structure.flags = flags;
  structure.pInheritanceInfo = nullptr;

  return structure;
}

VkFenceCreateInfo FenceCreateInfo(VkFenceCreateFlags flags) {
  return {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr, flags};
}

VkSubmitInfo SubmitInfo() {
  VkSubmitInfo structure;
  structure.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  structure.pNext = nullptr;

  return structure;
}

VkSamplerCreateInfo SamplerCreateInfo(
    VkFilter magFilter, VkFilter minFilter, VkSamplerMipmapMode mipmapMode,
    VkSamplerAddressMode addressModeU, VkSamplerAddressMode addressModeV,
    VkSamplerAddressMode addressModeW, float mipLodBias,
    VkBool32 anisotropyEnable, float maxAnisotropy, VkBool32 compareEnable,
    VkCompareOp compareOp, float minLod, float maxLod,
    VkBorderColor borderColor, VkBool32 unnormalizedCoordinates) {
  VkSamplerCreateInfo structure = {VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
                                   nullptr,
                                   0U,
                                   magFilter,
                                   minFilter,
                                   mipmapMode,
                                   addressModeU,
                                   addressModeV,
                                   addressModeW,
                                   mipLodBias,
                                   anisotropyEnable,
                                   maxAnisotropy,
                                   compareEnable,
                                   compareOp,
                                   minLod,
                                   maxLod,
                                   borderColor,
                                   unnormalizedCoordinates};

  return structure;
}

VkPipelineVertexInputStateCreateInfo PipelineVertexInputStateCreateInfo() {
  VkPipelineVertexInputStateCreateInfo structure;
  structure.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  structure.pNext = nullptr;
  structure.flags = 0U;

  return structure;
}

VkDeviceQueueCreateInfo DeviceQueueCreateInfo() {
  VkDeviceQueueCreateInfo structure;
  structure.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  structure.pNext = nullptr;
  structure.flags = 0U;

  return structure;
}

VkCommandPoolCreateInfo CommandPoolCreateInfo(VkCommandPoolCreateFlags flags) {
  VkCommandPoolCreateInfo structure;
  structure.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  structure.pNext = nullptr;
  structure.flags = flags;

  return structure;
}

VkShaderModuleCreateInfo ShaderModuleCreateInfo() {
  VkShaderModuleCreateInfo structure;
  structure.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  structure.pNext = nullptr;
  structure.flags = 0U;

  return structure;
}

VkComputePipelineCreateInfo ComputePipelineCreateInfo() {
  VkComputePipelineCreateInfo structure;
  structure.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  structure.pNext = nullptr;

  return structure;
}

VkPipelineShaderStageCreateInfo PipelineShaderStageCreateInfo() {
  VkPipelineShaderStageCreateInfo structure;
  structure.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  structure.pNext = nullptr;
  structure.flags = 0U;

  return structure;
}

// VkAttachmentDescription AttachmentDescription() {
// VkAttachmentDescription
//}

VkRenderPassCreateInfo RenderPassCreateInfo(
    uint32_t attachmentCount, const VkAttachmentDescription *pAttachments,
    uint32_t subpassCount, const VkSubpassDescription *pSubpasses,
    uint32_t dependencyCount, const VkSubpassDependency *pDependencies) {
  VkRenderPassCreateInfo structure = {VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
                                      nullptr,
                                      0U,
                                      attachmentCount,
                                      pAttachments,
                                      subpassCount,
                                      pSubpasses,
                                      dependencyCount,
                                      pDependencies};

  return structure;
}

VkDescriptorSetLayoutCreateInfo DescriptrorSetLayoutCreateInfo() {
  VkDescriptorSetLayoutCreateInfo structure;
  structure.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  structure.pNext = nullptr;
  structure.flags = 0U;

  return structure;
}

VkDescriptorPoolCreateInfo
DescriptrorPoolCreateInfo(uint32_t maxSets, uint32_t poolSizeCount,
                          const VkDescriptorPoolSize *pPoolSizes) {
  VkDescriptorPoolCreateInfo structure = {
      VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      nullptr,
      0U,
      maxSets,
      poolSizeCount,
      pPoolSizes};

  return structure;
}

VkDescriptorSetAllocateInfo
DescriptorSetAllocateInfo(VkDescriptorPool descriptorPool,
                          uint32_t descriptorSetCount,
                          const VkDescriptorSetLayout *pSetLayouts) {
  VkDescriptorSetAllocateInfo structure = {
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, nullptr, descriptorPool,
      descriptorSetCount, pSetLayouts};

  return structure;
}

VkWriteDescriptorSet
WriteDescriptorSet(VkDescriptorSet dstSet, uint32_t dstBinding,
                   uint32_t dstArrayElement, uint32_t descriptorCount,
                   VkDescriptorType descriptorType,
                   const VkDescriptorImageInfo *pImageInfo,
                   const VkDescriptorBufferInfo *pBufferInfo,
                   const VkBufferView *pTexelBufferView) {
  VkWriteDescriptorSet structure = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                    nullptr,
                                    dstSet,
                                    dstBinding,
                                    dstArrayElement,
                                    descriptorCount,
                                    descriptorType,
                                    pImageInfo,
                                    pBufferInfo,
                                    pTexelBufferView};

  return structure;
}

VkWriteDescriptorSet
WriteDescriptorSet(VkDescriptorSet dstSet, uint32_t dstBinding,
                   uint32_t dstArrayElement, uint32_t descriptorCount,
                   VkDescriptorType descriptorType,
                   const VkBufferView *pTexelBufferView,
                   const VkDescriptorImageInfo *pImageInfo,
                   const VkDescriptorBufferInfo *pBufferInfo) {
  VkWriteDescriptorSet structure = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                    nullptr,
                                    dstSet,
                                    dstBinding,
                                    dstArrayElement,
                                    descriptorCount,
                                    descriptorType,
                                    pImageInfo,
                                    pBufferInfo,
                                    pTexelBufferView};

  return structure;
}

VkWriteDescriptorSet
WriteDescriptorSet(VkDescriptorSet dstSet, uint32_t dstBinding,
                   uint32_t dstArrayElement, uint32_t descriptorCount,
                   VkDescriptorType descriptorType,
                   const VkDescriptorBufferInfo *pBufferInfo,
                   const VkBufferView *pTexelBufferView,
                   const VkDescriptorImageInfo *pImageInfo) {
  VkWriteDescriptorSet structure = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                    nullptr,
                                    dstSet,
                                    dstBinding,
                                    dstArrayElement,
                                    descriptorCount,
                                    descriptorType,
                                    pImageInfo,
                                    pBufferInfo,
                                    pTexelBufferView};

  return structure;
}
VkPipelineLayoutCreateInfo
PipelineLayoutCreateInfo(uint32_t setLayoutCount,
                         const VkDescriptorSetLayout *pSetLayouts,
                         uint32_t pushConstantRangeCount,
                         const VkPushConstantRange *pPushConstantRanges) {
  VkPipelineLayoutCreateInfo structure = {
      VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      nullptr,
      0U,
      setLayoutCount,
      pSetLayouts,
      pushConstantRangeCount,
      pPushConstantRanges};

  return structure;
}

VkFramebufferCreateInfo FramebufferCreateInfo(VkRenderPass renderPass,
                                              uint32_t attachmentCount,
                                              const VkImageView *pAttachments,
                                              uint32_t width, uint32_t height,
                                              uint32_t layers) {
  VkFramebufferCreateInfo structure = {
      VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
      nullptr,
      0U,
      renderPass,
      attachmentCount,
      pAttachments,
      width,
      height,
      layers};

  return structure;
}

VkImageMemoryBarrier
ImageMemoryBarrier(VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
                   VkImageLayout oldLayout, VkImageLayout newLayout,
                   uint32_t srcQueueFamilyIndex, uint32_t dstQueueFamilyIndex,
                   VkImage image, VkImageSubresourceRange subresourceRange) {
  VkImageMemoryBarrier structure = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                                    nullptr,
                                    srcAccessMask,
                                    dstAccessMask,
                                    oldLayout,
                                    newLayout,
                                    srcQueueFamilyIndex,
                                    dstQueueFamilyIndex,
                                    image,
                                    subresourceRange};

  return structure;
}
VkImageMemoryBarrier ImageMemoryBarrier() {
  VkImageMemoryBarrier structure;

  structure.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  structure.pNext = nullptr;

  return structure;
}

VkRenderPassBeginInfo RenderPassBeginInfo() {
  VkRenderPassBeginInfo structure;
  structure.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  structure.pNext = nullptr;

  return structure;
}

VkPipelineInputAssemblyStateCreateInfo PipelineInputAssemblyStateCreateInfo() {
  VkPipelineInputAssemblyStateCreateInfo structure;
  structure.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  structure.pNext = nullptr;
  structure.flags = 0U;

  return structure;
}

VkPipelineViewportStateCreateInfo PipelineViewportStateCreateInfo() {
  VkPipelineViewportStateCreateInfo structure;
  structure.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  structure.pNext = nullptr;
  structure.flags = 0U;

  return structure;
}

VkPipelineRasterizationStateCreateInfo PipelineRasterizationStateCreateInfo() {
  VkPipelineRasterizationStateCreateInfo structure;
  structure.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  structure.pNext = nullptr;
  structure.flags = 0U;

  return structure;
}

VkPipelineMultisampleStateCreateInfo PipelineMultisampleStateCreateInfo() {
  VkPipelineMultisampleStateCreateInfo structure;
  structure.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  structure.pNext = nullptr;
  structure.flags = 0U;

  return structure;
}

VkPipelineDepthStencilStateCreateInfo PipelineDepthStencilStateCreateInfo() {
  VkPipelineDepthStencilStateCreateInfo structure;
  structure.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  structure.pNext = nullptr;
  structure.flags = 0U;

  return structure;
}

VkPipelineColorBlendStateCreateInfo PipelineColorBlendStateCreateInfo() {
  VkPipelineColorBlendStateCreateInfo structure;
  structure.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  structure.pNext = nullptr;
  structure.flags = 0U;

  return structure;
}

VkGraphicsPipelineCreateInfo GraphicsPipelineCreateInfo() {
  VkGraphicsPipelineCreateInfo structure;
  structure.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  structure.pNext = nullptr;
  structure.flags = 0U;

  return structure;
}

VkPresentInfoKHR PresentInfoKHR() {
  VkPresentInfoKHR structure;
  structure.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  structure.pNext = nullptr;

  return structure;
}

VkAttachmentDescription
AttachmentDescription(VkAttachmentDescriptionFlags flags, VkFormat format,
                      VkSampleCountFlagBits samples, VkAttachmentLoadOp loadOp,
                      VkAttachmentStoreOp storeOp,
                      VkAttachmentLoadOp stencilLoadOp,
                      VkAttachmentStoreOp stencilStoreOp,
                      VkImageLayout initialLayout, VkImageLayout finalLayout) {
  VkAttachmentDescription structure = {
      flags,         format,         samples,       loadOp,     storeOp,
      stencilLoadOp, stencilStoreOp, initialLayout, finalLayout};

  return structure;
}

VkSubpassDescription SubpassDescription(
    VkPipelineBindPoint pipelineBindPoint, uint32_t inputAttachmentCount,
    const VkAttachmentReference *pInputAttachments,
    uint32_t colorAttachmentCount,
    const VkAttachmentReference *pColorAttachments,
    const VkAttachmentReference *pResolveAttachments,
    const VkAttachmentReference *pDepthStencilAttachment,
    uint32_t preserveAttachmentCount, const uint32_t *pPreserveAttachments) {
  VkSubpassDescription structure = {0U,
                                    pipelineBindPoint,
                                    inputAttachmentCount,
                                    pInputAttachments,
                                    colorAttachmentCount,
                                    pColorAttachments,
                                    pResolveAttachments,
                                    pDepthStencilAttachment,
                                    preserveAttachmentCount,
                                    pPreserveAttachments};

  return structure;
}

VkSubpassDependency SubpassDependency(uint32_t srcSubpass, uint32_t dstSubpass,
                                      VkPipelineStageFlags srcStageMask,
                                      VkPipelineStageFlags dstStageMask,
                                      VkAccessFlags srcAccessMask,
                                      VkAccessFlags dstAccessMask,
                                      VkDependencyFlags dependencyFlags) {
  VkSubpassDependency structure = {srcSubpass,     dstSubpass,    srcStageMask,
                                   dstStageMask,   srcAccessMask, dstAccessMask,
                                   dependencyFlags};

  return structure;
}

VkDescriptorPoolSize DescriptorPoolSize(VkDescriptorType type,
                                        uint32_t descriptorCount) {
  VkDescriptorPoolSize structure = {type, descriptorCount};

  return structure;
}

VkDescriptorSetLayoutBinding DescriptorSetLayoutBinding(
    uint32_t binding, VkDescriptorType descriptorType, uint32_t descriptorCount,
    VkShaderStageFlags stageFlags, const VkSampler *pImmutableSamplers) {
  VkDescriptorSetLayoutBinding structure = {
      binding, descriptorType, descriptorCount, stageFlags, pImmutableSamplers};

  return structure;
}

VkImageSubresourceRange ImageSubresourceRange(VkImageAspectFlags aspectMask,
                                              uint32_t baseMipLevel,
                                              uint32_t levelCount,
                                              uint32_t baseArrayLayer,
                                              uint32_t layerCount) {
  VkImageSubresourceRange structure = {aspectMask, baseMipLevel, levelCount,
                                       baseArrayLayer, layerCount};

  return structure;
}

VkPipelineColorBlendAttachmentState PipelineColorBlendAttachmentState(
    VkBool32 blendEnable, VkBlendFactor srcColorBlendFactor,
    VkBlendFactor dstColorBlendFactor, VkBlendOp colorBlendOp,
    VkBlendFactor srcAlphaBlendFactor, VkBlendFactor dstAlphaBlendFactor,
    VkBlendOp alphaBlendOp, VkColorComponentFlags colorWriteMask) {
  VkPipelineColorBlendAttachmentState structure = {
      blendEnable,  srcColorBlendFactor, dstColorBlendFactor,
      colorBlendOp, srcAlphaBlendFactor, dstAlphaBlendFactor,
      alphaBlendOp, colorWriteMask};

  return structure;
}

VkPipelineColorBlendStateCreateInfo PipelineColorBlendStateCreateInfo(
    VkBool32 logicOpEnable, VkLogicOp logicOp, uint32_t attachmentCount,
    const VkPipelineColorBlendAttachmentState *pAttachments,
    float *blendConstants) {
  VkPipelineColorBlendStateCreateInfo structure = {
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
      nullptr,
      0U,
      logicOpEnable,
      logicOp,
      attachmentCount,
      pAttachments,
      *blendConstants};

  return structure;
}

VkSpecializationMapEntry SpecializationMapEntry(uint32_t constantID,
                                                uint32_t offset, size_t size) {
  VkSpecializationMapEntry structure = {constantID, offset, size};

  return structure;
}

VkStencilOpState StencilOpState(VkStencilOp failOp, VkStencilOp passOp,
                                VkStencilOp depthFailOp, VkCompareOp compareOp,
                                uint32_t compareMask, uint32_t writeMask,
                                uint32_t reference) {
  return {failOp,      passOp,    depthFailOp, compareOp,
          compareMask, writeMask, reference};
}
VkImageBlit ImageBlit(VkImageSubresourceLayers srcSubresource,
                      VkOffset3D srcOffsets[2],
                      VkImageSubresourceLayers dstSubresource,
                      VkOffset3D dstOffsets[2]) {
  VkImageBlit structure;
  structure.srcSubresource = srcSubresource;
  structure.srcOffsets[0] = srcOffsets[0];
  structure.srcOffsets[1] = srcOffsets[1];
  structure.dstSubresource = dstSubresource;
  structure.dstOffsets[0] = dstOffsets[0];
  structure.dstOffsets[1] = dstOffsets[1];

  return structure;
}

} // namespace inits
} // namespace tools
} // namespace vks
