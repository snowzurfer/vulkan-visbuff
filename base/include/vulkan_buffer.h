#ifndef VKS_VULKANBUFFER
#define VKS_VULKANBUFFER

#include <vulkan/vulkan.h>

namespace vks {

class VulkanDevice;

struct VulkanBufferInitInfo {
  VulkanBufferInitInfo();

  VkBufferUsageFlags buffer_usage_flags;
  VkMemoryPropertyFlags memory_property_flags;
  VkDeviceSize size;
  VkCommandBuffer cmd_buff;
};

class VulkanBuffer {
public:
  VulkanBuffer();

  void Init(const VulkanDevice &device, const VulkanBufferInitInfo &info,
            const void *initial_data = nullptr);

  void Shutdown(const VulkanDevice &device);

  VkResult Map(const VulkanDevice &device, void **mapped_memory,
               VkDeviceSize size = VK_WHOLE_SIZE,
               VkDeviceSize offset = 0U) const;

  void Unmap(const VulkanDevice &device) const;

  const VkBuffer &buffer() const { return buffer_; };
  const VkDeviceMemory &memory() const { return memory_; };
  const VkDeviceSize size() const { return size_; };
  const VkDescriptorBufferInfo &descriptor() const { return descriptor_; };
  VkDeviceSize alignment() const { return alignment_; };
  VkBufferUsageFlags buffer_usage_flags() const { return buffer_usage_flags_; };
  VkMemoryPropertyFlags memory_property_flags() const {
    return memory_property_flags_;
  };

  VkDescriptorBufferInfo
  GetDescriptorBufferInfo(VkDeviceSize size = VK_WHOLE_SIZE,
                          VkDeviceSize offset = 0U) const;

private:
  VkBuffer buffer_;
  VkDeviceMemory memory_;
  VkDeviceSize size_;
  VkDescriptorBufferInfo descriptor_;
  VkDeviceSize alignment_;
  VkBufferUsageFlags buffer_usage_flags_;
  VkMemoryPropertyFlags memory_property_flags_;
  bool initialised_;

}; // class VulkanBuffer

} // namespace vks

#endif
