#include <EASTL/array.h>
#include <EASTL/unique_ptr.h>
#include <GLFW/glfw3.h>
#include <base_system.h>
#include <iostream>
#include <logger.hpp>
#include <vulkan_device.h>
#include <vulkan_image.h>
#include <vulkan_swapchain.h>
#include <vulkan_texture.h>
#include <vulkan_tools.h>

namespace vks {

extern const VkFormat kColourBufferFormat;

VulkanSwapChain::VulkanSwapChain()
    : images_(), surface_format_(), swapchain_(VK_NULL_HANDLE), width_(0U),
      height_(0U), current_idx_(0U) {}

void VulkanSwapChain::InitAndCreate(VkPhysicalDevice physical_device,
                                    const VulkanDevice &device,
                                    VkSurfaceKHR surface, const uint32_t width,
                                    const uint32_t height) {
  // First query for the formats and capabilites
  VkSurfaceCapabilitiesKHR surface_capabilities;
  VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
      physical_device, surface, &surface_capabilities));

  uint32_t surface_formats_count = 0U;
  if (vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface,
                                           &surface_formats_count,
                                           nullptr) != VK_SUCCESS ||
      surface_formats_count == 0U) {
    EXIT("Couldn't enumerate surface formats in VulkanSwapChain!");
  }

  std::vector<VkSurfaceFormatKHR> surface_formats(surface_formats_count);
  VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface,
                                                       &surface_formats_count,
                                                       surface_formats.data()));
#ifndef NDEBUG
  LOG("Surface formats count: " << surface_formats_count << ".");
  for (uint32_t i = 0U; i < surface_formats_count; i++) {
    LOG("Surface format " << i << " is: " << surface_formats[i].colorSpace
                          << ", " << surface_formats[i].format << ".");
  }
#endif
  uint32_t present_modes_count = 0U;
  if (vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface,
                                                &present_modes_count,
                                                nullptr) != VK_SUCCESS ||
      present_modes_count == 0U) {
    EXIT("Couldn't enumerate present modes in VulkanSwapChain!");
  }

  std::vector<VkPresentModeKHR> present_modes(present_modes_count);
  VK_CHECK_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(
      physical_device, surface, &present_modes_count, present_modes.data()));

#ifndef NDEBUG
  LOG("Present modes count: " << present_modes_count << ".");
  for (uint32_t i = 0U; i < present_modes_count; i++) {
    LOG("Present mode " << i << " is: " << present_modes[i] << ".");
  }
#endif
  // Then use this data to detemine the best configuration possible for the
  // swap chain

  width_ = width;
  height_ = height;

  uint32_t desired_number_of_images =
      tools::GetSwapChainNumImages(surface_capabilities);
  VkSurfaceFormatKHR desired_surface_format =
      tools::GetSwapChainFormat(kColourBufferFormat, surface_formats);
  VkExtent2D desired_image_extents =
      tools::GetSwapChainExtent(surface_capabilities, width_, height_);
  VkImageUsageFlags desired_usage =
      tools::GetSwapChainUsageFlags(surface_capabilities);
  VkSurfaceTransformFlagBitsKHR desired_transform =
      tools::GetSwapChainTransform(surface_capabilities);
  VkPresentModeKHR desired_present_mode =
      tools::GetSwapChainPresentMode(present_modes);
  VkSwapchainKHR old_swap_chain = swapchain_;

  // Check that certain capabilities and features are supported, if not
  // report and exit
  if (static_cast<int>(desired_usage) == -1) {
    EXIT("Desired usage not supported in VulkanSwapChain!");
  }
  if (static_cast<int>(desired_present_mode) == -1) {
    EXIT("Desired present mode not supported in VulkanSwapChain!");
  }

  if ((desired_image_extents.width == 0) ||
      (desired_image_extents.height == 0)) {
    // Current surface size is (0, 0) so we can't create a swap chain and render
    // anything (CanRender == false)
    // But we don't wont to kill the application as this situation may occur
    // i.e. when window gets minimized
    return;
  }

  VkSwapchainCreateInfoKHR swapchain_create_info = {
      VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
      nullptr,
      0U,
      surface,
      desired_number_of_images,
      desired_surface_format.format,
      desired_surface_format.colorSpace,
      desired_image_extents,
      1U,
      desired_usage,
      VK_SHARING_MODE_EXCLUSIVE,
      0,
      nullptr,
      desired_transform,
      VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
      desired_present_mode,
      VK_TRUE,
      old_swap_chain};

  VK_CHECK_RESULT(vkCreateSwapchainKHR(device.device(), &swapchain_create_info,
                                       nullptr, &swapchain_));

  uint32_t images_count = 0U;
  if (vkGetSwapchainImagesKHR(device.device(), swapchain_, &images_count,
                              nullptr) != VK_SUCCESS ||
      images_count == 0U) {
    EXIT("Couldn't get images from the swapchain in VulkanSwapChain!");
  }

  images_.resize(images_count);
  eastl::vector<VkImage> images(images_count);
  VK_CHECK_RESULT(vkGetSwapchainImagesKHR(device.device(), swapchain_,
                                          &images_count, images.data()));

  // Fill the swap chain buffer structures and create the image views
  uint32_t idx = 0U;
  for (eastl::vector<VkImage>::const_iterator i = images.begin();
       i != images.end(); ++i, ++idx) {

    // Create an image using the new struct
    VulkanImageAcquireInitInfo image_init_info;
    image_init_info.format = desired_surface_format.format;
    image_init_info.view_type = VK_IMAGE_VIEW_TYPE_2D;
    image_init_info.image = *i;
    image_init_info.image_usages = desired_usage;
    image_init_info.extents = {width_, height_, 1U};
    image_init_info.create_view = CreateView::YES;
    eastl::unique_ptr<VulkanImage> vks_image =
        eastl::make_unique<VulkanImage>();
    vks_image->Init(device, image_init_info);

    // Pass it to a vulkan texture, create vulkantexture
    VulkanTextureInitInfo texture_init_info;
    texture_init_info.image = eastl::move(vks_image);
    texture_init_info.create_sampler = CreateSampler::NO;
    texture_init_info.name.sprintf("swapchain_img_%d", idx);
    texture_init_info.sampler = VK_NULL_HANDLE;

    VulkanTexture *texture = nullptr;
    texture_manager()->CreateUniqueTexture(device, texture_init_info,
                                           texture_init_info.name, &texture);

    // Store returned ptr
    // VkImageViewCreateInfo image_view_create_info = {
    // VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
    // nullptr,
    // 0,
    // images[i],
    // VK_IMAGE_VIEW_TYPE_2D,
    // desired_surface_format.format,
    //{
    // VK_COMPONENT_SWIZZLE_IDENTITY,
    // VK_COMPONENT_SWIZZLE_IDENTITY,
    // VK_COMPONENT_SWIZZLE_IDENTITY,
    // VK_COMPONENT_SWIZZLE_IDENTITY
    //},
    //{
    // VK_IMAGE_ASPECT_COLOR_BIT,
    // 0U,
    // 1U,
    // 0U,
    // 1U
    //}
    //};

    // VK_CHECK_RESULT(vkCreateImageView(
    // device.device(),
    //&image_view_create_info,
    // nullptr,
    //&images_[i].view));

    images_[idx] = texture;
  }

  surface_format_ = desired_surface_format;
}

// Destroy and free Vulkan resources used by and for the swapchain
void VulkanSwapChain::Shutdown(const VulkanDevice &device) {
  if (swapchain_ != VK_NULL_HANDLE) {
    vkDestroySwapchainKHR(device.device(), swapchain_, nullptr);
    swapchain_ = VK_NULL_HANDLE;
  }

  // uint32_t num_images = SCAST_U32(images_.size());
  // for (uint32_t i = 0U; i < num_images; i++) {
  // vkDestroyImageView(device.device(), images_[i].view, nullptr);
  //}
  images_.clear();
}

void VulkanSwapChain::AcquireNextImage(const VulkanDevice &device,
                                       VkSemaphore present_semaphore,
                                       uint32_t &image_index) const {
  // By setting timeout to UINT64_MAX we will always wait
  // until the next image has been acquired or an actual error is thrown
  // With that we don't have to handle VK_NOT_READY
  VK_CHECK_RESULT(vkAcquireNextImageKHR(device.device(), swapchain_, UINT64_MAX,
                                        present_semaphore, VK_NULL_HANDLE,
                                        &image_index));

  current_idx_ = image_index;
}

uint32_t VulkanSwapChain::GetNumImages() const {
  return SCAST_U32(images_.size());
}

VkFormat VulkanSwapChain::GetSurfaceFormat() const {
  return surface_format_.format;
}

void VulkanSwapChain::Present(const VulkanQueue &queue,
                              VkSemaphore semaphore) const {
  VkPresentInfoKHR present_info = tools::inits::PresentInfoKHR();
  present_info.waitSemaphoreCount = 1U;
  present_info.pWaitSemaphores = &semaphore;
  present_info.swapchainCount = 1U;
  present_info.pSwapchains = &swapchain_;
  present_info.pImageIndices = &current_idx_;
  std::vector<VkResult> results(1U);
  present_info.pResults = results.data();

  VK_CHECK_RESULT(vkQueuePresentKHR(queue.queue, &present_info));
}

} // namespace vks
