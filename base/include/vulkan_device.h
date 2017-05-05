#ifndef VKS_VULKANDEVICE
#define VKS_VULKANDEVICE

#include <cstdint>
#include <vulkan/vulkan.h>

namespace vks {

struct VulkanQueue {
  uint32_t index;
  VkQueue queue;
  VkCommandPool cmd_pool;
};

class VulkanDevice {
public:
  VulkanDevice();

  void Init(VkInstance instance, VkSurfaceKHR surface);
  void Shutdown();

  VkDevice device() const { return device_; };
  VkPhysicalDevice physical_device() const { return physical_device_; };
  const VulkanQueue &graphics_queue() const { return graphics_queue_; };
  const VulkanQueue &present_queue() const { return present_queue_; };
  const VulkanQueue &compute_queue() const { return compute_queue_; };
  const VkPhysicalDeviceProperties physical_properties() const {
    return physical_properties_;
  };
  VkFormat depth_format() const { return depth_format_; };
  uint32_t GetGraphicsQueueIndex() const { return graphics_queue_.index; };
  uint32_t GetPresentQueueIndex() const { return present_queue_.index; };
  uint32_t GetComputeQueueIndex() const { return compute_queue_.index; };

  // Get an index to the a type of memory which respects as close as possible
  // the properties and type passed as parameters
  uint32_t GetMemoryType(uint32_t type_bits,
                         VkMemoryPropertyFlags properties_flags) const;

  // Whether the logical device has been created and/or is still valid
  bool IsDeviceVaild() const { return device_ != VK_NULL_HANDLE; };

  // Create the view for a given image
  void CreateImageView(const VkImageViewCreateInfo &image_vew_create_info,
                       class VulkanImage &image) const;

private:
  VkPhysicalDevice physical_device_;
  VkDevice device_;
  VulkanQueue graphics_queue_;
  VulkanQueue present_queue_;
  VulkanQueue compute_queue_;
  VkPhysicalDeviceProperties physical_properties_;
  VkPhysicalDeviceFeatures physical_features_;
  VkPhysicalDeviceMemoryProperties physical_memory_properties_;
  VkFormat depth_format_;

  // Whether a physical device supports the necessary features for the
  // application
  bool IsPhysicalDeviceSuitable(VkPhysicalDevice physical_device,
                                struct QueueFamilyIndices &queue_families,
                                VkSurfaceKHR surface) const;

}; // class VulkanDevice

} // namespace vks

#endif
