#ifndef VKS_SWAPCHAIN
#define VKS_SWAPCHAIN

#include <EASTL/vector.h>
#include <vulkan/vulkan.h>

namespace vks {

class VulkanDevice;
struct VulkanQueue;
class VulkanTexture;

typedef struct {
  VkImage handle;
  VkImageView view;
} SwapChainBuffer;

class VulkanSwapChain {
public:
  VulkanSwapChain();

  // Initialise the swap chain, and also create it. Will destroy the old one
  // if present and create a new one.
  // The width and height may be adjusted to fit the requirements of the
  // swapchain
  void InitAndCreate(VkPhysicalDevice physical_device,
                     const VulkanDevice &device, VkSurfaceKHR surface,
                     const uint32_t width, const uint32_t height);

  // Create the swapchain. Will destroy the old one
  // if present and create a new one.
  // The width and height may be adjusted to fit the requirements of the
  // swapchain
  // bool Create(uint32_t &width, uint32_t &height);

  // Destroy and free Vulkan resources used by and for the swapchain
  void Shutdown(const VulkanDevice &device);

  // Get the index to the next available image from the swapchain in
  // image_index.
  // It waits on present_semaphore to acquire it
  void AcquireNextImage(const VulkanDevice &device,
                        VkSemaphore present_semaphore,
                        uint32_t &image_index) const;

  void Present(const VulkanQueue &queue, VkSemaphore semaphore) const;

  friend class VulkanBase;

  uint32_t GetNumImages() const;

  const eastl::vector<VulkanTexture *> &images() const { return images_; }

  VkFormat GetSurfaceFormat() const;

  uint32_t width() const { return width_; }
  uint32_t height() const { return height_; }

private:
  eastl::vector<VulkanTexture *> images_;
  VkSurfaceFormatKHR surface_format_;
  VkSwapchainKHR swapchain_;
  uint32_t width_, height_;
  mutable uint32_t current_idx_;
}; // class VulkanSwapChain

} // namespace vks

#endif
