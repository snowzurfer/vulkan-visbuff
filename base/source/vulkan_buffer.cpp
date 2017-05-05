#include <logger.hpp>
#include <utility>
#include <vulkan_buffer.h>
#include <vulkan_device.h>
#include <vulkan_tools.h>

namespace vks {

VulkanBuffer::VulkanBuffer()
    : buffer_(VK_NULL_HANDLE), memory_(VK_NULL_HANDLE), size_(0U),
      descriptor_(), alignment_(0U), buffer_usage_flags_(),
      memory_property_flags_(), initialised_(false) {}

void VulkanBuffer::Init(const VulkanDevice &device,
                        const VulkanBufferInitInfo &info,
                        const void *initial_data) {
  if (initialised_ == true) {
    Shutdown(device);
  }

  size_ = info.size;
  buffer_usage_flags_ = info.buffer_usage_flags;
  memory_property_flags_ = info.memory_property_flags;

  if (!(info.memory_property_flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) {
    buffer_usage_flags_ |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  }

  // Create a buffer of the given size
  VkBufferCreateInfo buffer_create_info = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                                           nullptr,
                                           0U,
                                           info.size,
                                           buffer_usage_flags_,
                                           VK_SHARING_MODE_EXCLUSIVE,
                                           0U,
                                           nullptr};

  VK_CHECK_RESULT(
      vkCreateBuffer(device.device(), &buffer_create_info, nullptr, &buffer_));

  // Check what the memory requirements for the buffer are
  VkMemoryRequirements memory_requirements;
  vkGetBufferMemoryRequirements(device.device(), buffer_, &memory_requirements);

  // Then allocate the required memory
  VkMemoryAllocateInfo memory_alloc_info = {
      VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, nullptr, memory_requirements.size,
      device.GetMemoryType(memory_requirements.memoryTypeBits,
                           memory_property_flags_)};

  VK_CHECK_RESULT(
      vkAllocateMemory(device.device(), &memory_alloc_info, nullptr, &memory_));
  // Assign the memory to the buffer
  VK_CHECK_RESULT(vkBindBufferMemory(device.device(), buffer_, memory_, 0));

  if (initial_data != nullptr) {
    if (info.memory_property_flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
      void *mapped = nullptr;
      vkMapMemory(device.device(), memory_, 0, size_, 0, &mapped);
      memcpy(mapped, initial_data, size_);
      vkUnmapMemory(device.device(), memory_);
    } else if (info.memory_property_flags &
               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
      VKS_ASSERT(info.cmd_buff != VK_NULL_HANDLE, "Must pass a cmd buffer \
                 to perform copy from staging buffer to device buffer!");
      // Create a host-visible staging buffer for containing the raw data
      VkBufferCreateInfo buf_create_info = tools::inits::BufferCreateInfo();
      buf_create_info.size = size_;
      buf_create_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
      buf_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
      buf_create_info.queueFamilyIndexCount = 0U;
      buf_create_info.pQueueFamilyIndices = nullptr;

      VkBuffer staging_buffer = VK_NULL_HANDLE;
      VK_CHECK_RESULT(vkCreateBuffer(device.device(), &buf_create_info, nullptr,
                                     &staging_buffer));

      // Get the memory requirements for the staging buffer
      VkMemoryRequirements memory_requirements;
      vkGetBufferMemoryRequirements(device.device(), staging_buffer,
                                    &memory_requirements);

      // Allocate memory for the staging buffer
      VkMemoryAllocateInfo mem_alloc_info = tools::inits::MemoryAllocateInfo();
      mem_alloc_info.allocationSize = memory_requirements.size;
      // Retrieve the index for a type of memory visible to the host
      mem_alloc_info.memoryTypeIndex =
          device.GetMemoryType(memory_requirements.memoryTypeBits,
                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

      VkDeviceMemory staging_buffer_memory = VK_NULL_HANDLE;
      VK_CHECK_RESULT(vkAllocateMemory(device.device(), &mem_alloc_info,
                                       nullptr, &staging_buffer_memory));

      // Then bind it to the buffer handle
      VK_CHECK_RESULT(vkBindBufferMemory(device.device(), staging_buffer,
                                         staging_buffer_memory, 0U));

      // Now copy the texture data into the staging buffer
      void *data = nullptr;
      VK_CHECK_RESULT(vkMapMemory(device.device(), staging_buffer_memory, 0U,
                                  memory_requirements.size, 0U, &data));
      memcpy(data, initial_data, size_);
      vkUnmapMemory(device.device(), staging_buffer_memory);

      // Setup buffer copy regions
      std::vector<VkBufferCopy> buffer_copy_regions;
      VkBufferCopy buff_copy;
      buff_copy.srcOffset = 0U;
      buff_copy.dstOffset = 0U;
      buff_copy.size = size_;
      buffer_copy_regions.push_back(buff_copy);

      // Use the separate command buffer for texture loading
      VkCommandBufferBeginInfo cmd_buff_begin_info =
          tools::inits::CommandBufferBeginInfo();
      VK_CHECK_RESULT(
          vkBeginCommandBuffer(info.cmd_buff, &cmd_buff_begin_info));
      vkCmdCopyBuffer(info.cmd_buff, staging_buffer, buffer_,
                      SCAST_U32(buffer_copy_regions.size()),
                      buffer_copy_regions.data());

      VK_CHECK_RESULT(vkEndCommandBuffer(info.cmd_buff));

      // Use a fence to make sure that the copying operations have completed
      // and then submit the command buffer
      VkFenceCreateInfo fence_create_info = tools::inits::FenceCreateInfo();
      VkFence copy_fence = VK_NULL_HANDLE;
      VK_CHECK_RESULT(vkCreateFence(device.device(), &fence_create_info,
                                    nullptr, &copy_fence));

      VkSubmitInfo submit_info = tools::inits::SubmitInfo();
      submit_info.waitSemaphoreCount = 0U;
      submit_info.pWaitSemaphores = nullptr;
      submit_info.pWaitDstStageMask = nullptr;
      submit_info.commandBufferCount = 1U;
      submit_info.pCommandBuffers = &info.cmd_buff;
      submit_info.signalSemaphoreCount = 0U;
      submit_info.pSignalSemaphores = nullptr;

      vkQueueSubmit(device.graphics_queue().queue, 1U, &submit_info,
                    copy_fence);

      VK_CHECK_RESULT(vkWaitForFences(device.device(), 1U, &copy_fence, VK_TRUE,
                                      UINT64_MAX));

      // Cleanup the staging resources
      vkDestroyFence(device.device(), copy_fence, nullptr);
      copy_fence = VK_NULL_HANDLE;
      vkFreeMemory(device.device(), staging_buffer_memory, nullptr);
      staging_buffer_memory = VK_NULL_HANDLE;
      vkDestroyBuffer(device.device(), staging_buffer, nullptr);
      staging_buffer = VK_NULL_HANDLE;
    }
  }

  alignment_ = memory_requirements.alignment;

  initialised_ = true;

  LOG("Initialised buffer " << buffer_);
}

void VulkanBuffer::Shutdown(const VulkanDevice &device) {
  LOG("Shutdown buffer " << buffer_);
  if (buffer_ != VK_NULL_HANDLE) {
    vkDestroyBuffer(device.device(), buffer_, nullptr);
    buffer_ = VK_NULL_HANDLE;
  }
  if (memory_ != VK_NULL_HANDLE) {
    vkFreeMemory(device.device(), memory_, nullptr);
    memory_ = VK_NULL_HANDLE;
  }

  initialised_ = false;
}

VkResult VulkanBuffer::Map(const VulkanDevice &device, void **mapped_memory,
                           VkDeviceSize size, VkDeviceSize offset) const {
  return vkMapMemory(device.device(), memory_, offset, size, 0U, mapped_memory);
}

void VulkanBuffer::Unmap(const VulkanDevice &device) const {
  vkUnmapMemory(device.device(), memory_);
}

VkDescriptorBufferInfo
VulkanBuffer::GetDescriptorBufferInfo(VkDeviceSize size,
                                      VkDeviceSize offset) const {
  VkDescriptorBufferInfo info;
  info.offset = offset;
  info.range = size;
  info.buffer = buffer_;

  return info;
}

VulkanBufferInitInfo::VulkanBufferInitInfo()
    : buffer_usage_flags(), memory_property_flags(), size(0U),
      cmd_buff(VK_NULL_HANDLE) {}

} // namespace vks
