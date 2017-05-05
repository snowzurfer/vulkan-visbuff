// Class which holds base functionality offered by Vulkan for easy usage by the
// samples

#ifndef VKS_VULKANBASE
#define VKS_VULKANBASE

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <vulkan_buffer.h>
#include <cstdint>
#include <eastl/vector.h>
#include <vulkan_device.h>
#include <vulkan_swapchain.h>

namespace vks {

class VulkanBase {
public:
  VulkanBase();

  // Initialise the class; not called by the ctor; the width and height
  // might be adjusted depending on the features of the swapchain
  void Init(GLFWwindow *window, const uint32_t width, const uint32_t height);
  // Clear-up the class; not called by the dtor
  void Shutdown();

  const VulkanDevice &device() const { return device_; }

  const VulkanSwapChain &swapchain() const { return swapchain_; }

  const eastl::vector<VkCommandBuffer> &pre_present_cmd_buffers() const {
    return pre_present_cmd_buffers_;
  }
  const eastl::vector<VkCommandBuffer> &post_present_cmd_buffers() const {
    return post_present_cmd_buffers_;
  }
  const eastl::vector<VkCommandBuffer> &graphics_queue_cmd_buffers() const {
    return graphics_queue_cmd_buffers_;
  }

  void ResetGraphicsCmdBbuffer();

  VkSemaphore image_available_semaphore() const {
    return image_available_semaphore_;
  }
  VkSemaphore rendering_finished_semaphore() const {
    return rendering_finished_semaphore_;
  }

  VkCommandBuffer copy_cmd_buff() const { return copy_cmd_buff_; }

private:
  // Create application-wide Vulkan instance
  void CreateInstance();
  // Create application-wide Vulkan device
  void CreateDevice();
  // Create base semaphores
  void CreateBaseSemaphores();
  // Create the window surface to blit rendering results to
  void CreateSurface(GLFWwindow *window);
  // Create the swap chain for presenting images; the width and height
  // might be adjusted depending on the features of the swapchain
  void CreateSwapChain(const uint32_t width, const uint32_t height);
  // Create the base command buffers
  void CreateBaseCmdBuffers();
  // Record the base command buffers
  void RecordBaseCmdBuffers();
  // Setup validation layers callback if in debug mode
  void CreateCallback();

  VkInstance instance_;
  // Maybe could move the two semaphores, or just rendering finished, in
  // VulkanSwapchain
  VkSemaphore image_available_semaphore_;
  VkSemaphore rendering_finished_semaphore_;
  eastl::vector<VkCommandBuffer> pre_present_cmd_buffers_;
  eastl::vector<VkCommandBuffer> post_present_cmd_buffers_;
  eastl::vector<VkCommandBuffer> graphics_queue_cmd_buffers_;
  VkCommandBuffer copy_cmd_buff_;
  VkDebugReportCallbackEXT callback_;
  VkSurfaceKHR surface_;
  VulkanSwapChain swapchain_;
  VulkanDevice device_;

}; // class VulkanBase

} // namespace vks

#endif
