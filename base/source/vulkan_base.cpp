#include <EASTL/vector.h>
#include <cstring>
#include <iostream>
#include <logger.hpp>
#include <set>
#include <vulkan_base.h>
#include <vulkan_tools.h>

namespace vks {

#ifndef NDEBUG
static const eastl::vector<const char *> kInstanceDebugExtensions = {
    VK_EXT_DEBUG_REPORT_EXTENSION_NAME};

static const eastl::vector<const char *> kInstanceDebugValidationLayers = {
    "VK_LAYER_LUNARG_standard_validation"};
#endif

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugReportCallback(
    VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT object_type,
    uint64_t object, size_t location, int32_t message_code,
    const char *p_layer_prefix, const char *p_message, void *p_user_data);

VulkanBase::VulkanBase()
    : instance_(VK_NULL_HANDLE), image_available_semaphore_(VK_NULL_HANDLE),
      rendering_finished_semaphore_(VK_NULL_HANDLE),
      pre_present_cmd_buffers_(VK_NULL_HANDLE),
      post_present_cmd_buffers_(VK_NULL_HANDLE),
      graphics_queue_cmd_buffers_(VK_NULL_HANDLE),
      copy_cmd_buff_(VK_NULL_HANDLE), callback_(VK_NULL_HANDLE),
      surface_(VK_NULL_HANDLE), swapchain_(), device_() {}

void VulkanBase::Init(GLFWwindow *window, const uint32_t width,
                      const uint32_t height) {
  CreateInstance();
  CreateCallback();
  CreateSurface(window);
  CreateDevice();
  CreateBaseSemaphores();
  CreateSwapChain(width, height);
  CreateBaseCmdBuffers();
}

void VulkanBase::Shutdown() {
  if (device_.IsDeviceVaild()) {
    vkDeviceWaitIdle(device_.device());
    if (copy_cmd_buff_ != VK_NULL_HANDLE) {
      vkFreeCommandBuffers(device_.device(), device_.graphics_queue().cmd_pool,
                           1U, &copy_cmd_buff_);
      copy_cmd_buff_ = VK_NULL_HANDLE;
    }
    if (graphics_queue_cmd_buffers_.size() > 0) {
      vkFreeCommandBuffers(device_.device(), device_.graphics_queue().cmd_pool,
                           SCAST_U32(graphics_queue_cmd_buffers_.size()),
                           graphics_queue_cmd_buffers_.data());
      graphics_queue_cmd_buffers_.clear();
    }
    if (post_present_cmd_buffers_.size() > 0) {
      vkFreeCommandBuffers(device_.device(), device_.present_queue().cmd_pool,
                           SCAST_U32(post_present_cmd_buffers_.size()),
                           post_present_cmd_buffers_.data());
      post_present_cmd_buffers_.clear();
    }
    if (pre_present_cmd_buffers_.size() > 0) {
      vkFreeCommandBuffers(device_.device(), device_.present_queue().cmd_pool,
                           SCAST_U32(pre_present_cmd_buffers_.size()),
                           pre_present_cmd_buffers_.data());
      pre_present_cmd_buffers_.clear();
    }
    if (rendering_finished_semaphore_ != VK_NULL_HANDLE) {
      vkDestroySemaphore(device_.device(), rendering_finished_semaphore_,
                         nullptr);
      rendering_finished_semaphore_ = VK_NULL_HANDLE;
    }
    if (image_available_semaphore_ != VK_NULL_HANDLE) {
      vkDestroySemaphore(device_.device(), image_available_semaphore_, nullptr);
      image_available_semaphore_ = VK_NULL_HANDLE;
    }

    swapchain_.Shutdown(device_);

    device_.Shutdown();

    if (callback_ != VK_NULL_HANDLE && instance_ != VK_NULL_HANDLE) {
      PFN_vkDestroyDebugReportCallbackEXT vkDestroyDebugReportCallbackEXT =
          reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>(
              vkGetInstanceProcAddr(instance_,
                                    "vkDestroyDebugReportCallbackEXT"));
      vkDestroyDebugReportCallbackEXT(instance_, callback_, nullptr);
    }

    if (surface_ != VK_NULL_HANDLE && instance_ != VK_NULL_HANDLE) {
      vkDestroySurfaceKHR(instance_, surface_, nullptr);
      surface_ = VK_NULL_HANDLE;
    }

    if (instance_ != VK_NULL_HANDLE) {
      vkDestroyInstance(instance_, nullptr);
      instance_ = VK_NULL_HANDLE;
    }
  }
}

void VulkanBase::CreateInstance() {
  VkApplicationInfo application_info = {VK_STRUCTURE_TYPE_APPLICATION_INFO,
                                        nullptr,
                                        "vulkan-sagres",
                                        VK_MAKE_VERSION(1, 0, 0),
                                        "vulkan-sagres",
                                        VK_MAKE_VERSION(1, 0, 0),
                                        VK_MAKE_VERSION(1, 0, 0)};

  uint32_t glfw_extension_count = 0U;
  const char **glfw_extensions;
  glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

  eastl::vector<const char *> extensions;
  extensions.assign(glfw_extensions, glfw_extensions + glfw_extension_count);

  eastl::vector<const char *> layers;

#ifndef NDEBUG
  // Use validation layers
  layers.assign(kInstanceDebugValidationLayers.begin(),
                kInstanceDebugValidationLayers.end());
  extensions.insert(extensions.end(), kInstanceDebugExtensions.begin(),
                    kInstanceDebugExtensions.end());
#endif

  VkInstanceCreateInfo instance_create_info = {
      VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      nullptr,
      0,
      &application_info,
      SCAST_U32(layers.size()),
      layers.data(),
      SCAST_U32(extensions.size()),
      extensions.data()};

  VK_CHECK_RESULT(vkCreateInstance(&instance_create_info, nullptr, &instance_));
}

void VulkanBase::CreateDevice() { device_.Init(instance_, surface_); }

void VulkanBase::CreateSurface(GLFWwindow *window) {
  if (glfwCreateWindowSurface(instance_, window, nullptr, &surface_) !=
      VK_SUCCESS) {
    EXIT("Could not create GLFW surface!");
  }
}

void VulkanBase::CreateBaseSemaphores() {
  VkSemaphoreCreateInfo semaphore_create_info = {
      VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, nullptr, 0U};

  VK_CHECK_RESULT(vkCreateSemaphore(device_.device(), &semaphore_create_info,
                                    nullptr, &image_available_semaphore_));
  VK_CHECK_RESULT(vkCreateSemaphore(device_.device(), &semaphore_create_info,
                                    nullptr, &rendering_finished_semaphore_));
}

void VulkanBase::CreateSwapChain(const uint32_t width, const uint32_t height) {
  swapchain_.InitAndCreate(device_.physical_device(), device_, surface_, width,
                           height);
}

void VulkanBase::CreateBaseCmdBuffers() {
  pre_present_cmd_buffers_.resize(swapchain_.images_.size());
  post_present_cmd_buffers_.resize(swapchain_.images_.size());
  graphics_queue_cmd_buffers_.resize(swapchain_.images_.size());

  VkCommandBufferAllocateInfo cmd_buffer_allocate_info = {
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, nullptr,
      device_.present_queue().cmd_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      SCAST_U32(pre_present_cmd_buffers_.size())};

  VK_CHECK_RESULT(vkAllocateCommandBuffers(device_.device(),
                                           &cmd_buffer_allocate_info,
                                           pre_present_cmd_buffers_.data()));

  VK_CHECK_RESULT(vkAllocateCommandBuffers(device_.device(),
                                           &cmd_buffer_allocate_info,
                                           post_present_cmd_buffers_.data()));

  cmd_buffer_allocate_info.commandPool = device_.graphics_queue().cmd_pool;
  VK_CHECK_RESULT(vkAllocateCommandBuffers(device_.device(),
                                           &cmd_buffer_allocate_info,
                                           graphics_queue_cmd_buffers_.data()));

  cmd_buffer_allocate_info.commandBufferCount = 1U;
  VK_CHECK_RESULT(vkAllocateCommandBuffers(
      device_.device(), &cmd_buffer_allocate_info, &copy_cmd_buff_));
}

void VulkanBase::RecordBaseCmdBuffers() {
  // VkImageSubresourceRange image_subresource_range;
  // image_subresource_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  // image_subresource_range.baseMipLevel = 0U;
  // image_subresource_range.levelCount = 1U;
  // image_subresource_range.baseArrayLayer = 0U;
  // image_subresource_range.layerCount = 1U;

  // uint32_t images_count = SCAST_U32(swapchain_.images_.size());
  // for (uint32_t i = 0U; i < images_count; ++i) {
  //// Create a command buffer for a post-present barrier; it transforms the
  //// image back to a colour attachment so that the render pass can write to
  //// it. Use an undefined image layout because it doesn't matter what was in
  //// the previous image

  ////vkBeginCommandBuffer(post_present_cmd_buffers_[i], &cmd_buff_begin_info);

  ////if (device_.GetPresentQueueIndex() != device_.GetGraphicsQueueIndex()) {
  ////LOG("Present and graphics queues are different! Perform image barrier.");
  ////tools::SetImageMemoryBarrier(post_present_cmd_buffers_[i],
  ////swapchain_.images_[i].handle,
  ////device_.GetPresentQueueIndex(),
  ////device_.GetGraphicsQueueIndex(),
  ////VK_ACCESS_MEMORY_READ_BIT,
  ////VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
  ////VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
  ////VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
  ////image_subresource_range);
  ////}

  ////vkEndCommandBuffer(post_present_cmd_buffers_[i]);

  ////// Create a command buffer for a post-present barrier; it transforms the
  ////// image back to a colour attachment so that the render pass can write to
  ////// it. Use an undefined image layout because it doesn't matter what was in
  ////// the previous image

  ////vkBeginCommandBuffer(pre_present_cmd_buffers_[i], &cmd_buff_begin_info);

  ////if (device_.GetPresentQueueIndex() != device_.GetGraphicsQueueIndex()) {
  ////LOG("Present and graphics queues are different! Perform image barrier.");
  ////tools::SetImageMemoryBarrier(post_present_cmd_buffers_[i],
  ////swapchain_.images_[i].handle,
  ////device_.GetGraphicsQueueIndex(),
  ////device_.GetPresentQueueIndex(),
  ////VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
  ////VK_ACCESS_MEMORY_READ_BIT,
  ////VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
  ////VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
  ////image_subresource_range);
  ////}

  // vkEndCommandBuffer(pre_present_cmd_buffers_[i]);
  //}
}

void VulkanBase::CreateCallback() {
#ifndef NDEBUG
  // Load VK_EXT_debug_report entry points in debug builds
  PFN_vkCreateDebugReportCallbackEXT vkCreateDebugReportCallbackEXT =
      reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(
          vkGetInstanceProcAddr(instance_, "vkCreateDebugReportCallbackEXT"));

  // Setup the callback creation information
  VkDebugReportCallbackCreateInfoEXT callback_create_info = {
      VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT, nullptr,
      VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT |
          VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT,
      &DebugReportCallback, nullptr};

  // Register the callback
  VK_CHECK_RESULT(vkCreateDebugReportCallbackEXT(
      instance_, &callback_create_info, nullptr, &callback_));
#endif
}

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugReportCallback(
    VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT object_type,
    uint64_t object, size_t location, int32_t message_code,
    const char *p_layer_prefix, const char *p_message, void *p_user_data) {
  if (flags == VK_DEBUG_REPORT_INFORMATION_BIT_EXT) {
    LOG(p_message);
  }
  if (flags == VK_DEBUG_REPORT_ERROR_BIT_EXT) {
    LOG_ERR(p_message);
  }
  if (flags == VK_DEBUG_REPORT_WARNING_BIT_EXT ||
      flags == VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT) {
    LOG_WARN(p_message);
  }

  return VK_FALSE;
}

void VulkanBase::ResetGraphicsCmdBbuffer() {
  uint32_t num_cmd_buffs = SCAST_U32(graphics_queue_cmd_buffers_.size());
  for (uint32_t i = 0U; i < num_cmd_buffs; i++) {
    LOG("Cmd buff num " << i << ": " << graphics_queue_cmd_buffers_[i]);
    VK_CHECK_RESULT(
        vkResetCommandBuffer(graphics_queue_cmd_buffers_[i],
                             VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT));
  }
}

} // namespace vks
