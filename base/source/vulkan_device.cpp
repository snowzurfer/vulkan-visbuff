#include <cstring>
#include <iostream>
#include <logger.hpp>
#include <set>
#include <sstream>
#include <vector>
#include <vulkan_buffer.h>
#include <vulkan_device.h>
#include <vulkan_image.h>
#include <vulkan_tools.h>

static const std::vector<const char *> kDeviceExtensions = {
    "VK_AMD_shader_explicit_vertex_parameter", VK_KHR_SWAPCHAIN_EXTENSION_NAME};

#ifndef NDEBUG
static const std::vector<const char *> kDeviceDebugValidationLayers = {
    "VK_LAYER_LUNARG_standard_validation"};
#endif

namespace vks {

struct QueueFamilyIndices {
  uint32_t graphics_family;
  uint32_t present_family;
  uint32_t compute_family;
};

static bool
IsQueueFamilyIndicesComplete(const QueueFamilyIndices &family_indices);

VulkanDevice::VulkanDevice()
    : physical_device_(VK_NULL_HANDLE), device_(VK_NULL_HANDLE),
      graphics_queue_(), present_queue_(), compute_queue_(),
      physical_properties_(), physical_features_(),
      physical_memory_properties_(), depth_format_() {}

void VulkanDevice::Init(VkInstance instance, VkSurfaceKHR surface) {
  uint32_t num_devices = 0U;
  VK_CHECK_RESULT(vkEnumeratePhysicalDevices(instance, &num_devices, nullptr));
  if (num_devices == 0U) {
    EXIT("No physical devices identified!");
  }

  std::vector<VkPhysicalDevice> physical_devices(num_devices);
  VK_CHECK_RESULT(vkEnumeratePhysicalDevices(instance, &num_devices,
                                             physical_devices.data()));

  // This saves both the physical device which we want to use and the queue
  // family indices which we want to create queues from
  QueueFamilyIndices queue_families = {UINT32_MAX, UINT32_MAX, UINT32_MAX};
  for (uint32_t i = 0; i < num_devices; ++i) {
    if (IsPhysicalDeviceSuitable(physical_devices[i], queue_families,
                                 surface)) {
      physical_device_ = physical_devices[i];
      break;
    }
  }

  if (physical_device_ == VK_NULL_HANDLE) {
    EXIT("No suitable physical device!");
  }

  graphics_queue_.index = queue_families.graphics_family;
  present_queue_.index = queue_families.present_family;
  compute_queue_.index = queue_families.compute_family;

  // Store properties and features of the physical device for later use
  vkGetPhysicalDeviceProperties(physical_device_, &physical_properties_);
  vkGetPhysicalDeviceFeatures(physical_device_, &physical_features_);
  vkGetPhysicalDeviceMemoryProperties(physical_device_,
                                      &physical_memory_properties_);
  tools::GetSupportedDepthFormat(physical_device_, depth_format_);

  std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
  // Use a set to select only unique family ids
  std::set<uint32_t> unique_queue_families = {queue_families.graphics_family,
                                              queue_families.present_family,
                                              queue_families.compute_family};

  std::set<uint32_t>::iterator it;
  // queue_priorities is a vector so that if in the future there is the need
  // to create more queues for a given family, it can be expanded
  std::vector<float> queue_priorities = {1.0f};
  for (it = unique_queue_families.begin(); it != unique_queue_families.end();
       it++) {

    VkDeviceQueueCreateInfo queue_create_info =
        tools::inits::DeviceQueueCreateInfo();
    queue_create_info.queueFamilyIndex = *it;
    queue_create_info.queueCount = SCAST_U32(queue_priorities.size());
    queue_create_info.pQueuePriorities = queue_priorities.data();

    queue_create_infos.push_back(queue_create_info);
  }

  std::vector<const char *> layers;
  std::vector<const char *> extensions;
  extensions.assign(kDeviceExtensions.begin(), kDeviceExtensions.end());

#ifndef NDEBUG
  layers.assign(kDeviceDebugValidationLayers.begin(),
                kDeviceDebugValidationLayers.end());
#endif

  VkDeviceCreateInfo device_create_info = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
                                           nullptr,
                                           0,
                                           SCAST_U32(queue_create_infos.size()),
                                           queue_create_infos.data(),
                                           SCAST_U32(layers.size()),
                                           layers.data(),
                                           SCAST_U32(extensions.size()),
                                           extensions.data(),
                                           &physical_features_};

  VK_CHECK_RESULT(
      vkCreateDevice(physical_device_, &device_create_info, nullptr, &device_));

  // Retrieve queues after having created the device
  vkGetDeviceQueue(device_, queue_families.graphics_family, 0U,
                   &graphics_queue_.queue);
  vkGetDeviceQueue(device_, queue_families.present_family, 0U,
                   &present_queue_.queue);
  vkGetDeviceQueue(device_, queue_families.compute_family, 0U,
                   &compute_queue_.queue);

  // Create default command pools for the queues
  VkCommandPoolCreateInfo cmd_pool_create_info =
      tools::inits::CommandPoolCreateInfo(
          VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
  cmd_pool_create_info.queueFamilyIndex = queue_families.graphics_family;
  VK_CHECK_RESULT(vkCreateCommandPool(device_, &cmd_pool_create_info, nullptr,
                                      &graphics_queue_.cmd_pool));
  cmd_pool_create_info.queueFamilyIndex = queue_families.present_family;
  VK_CHECK_RESULT(vkCreateCommandPool(device_, &cmd_pool_create_info, nullptr,
                                      &present_queue_.cmd_pool));
  cmd_pool_create_info.queueFamilyIndex = queue_families.compute_family;
  VK_CHECK_RESULT(vkCreateCommandPool(device_, &cmd_pool_create_info, nullptr,
                                      &compute_queue_.cmd_pool));
}

void VulkanDevice::Shutdown() {
  if (compute_queue_.cmd_pool != VK_NULL_HANDLE) {
    vkDestroyCommandPool(device_, compute_queue_.cmd_pool, nullptr);
    compute_queue_.cmd_pool = VK_NULL_HANDLE;
  }
  if (present_queue_.cmd_pool != VK_NULL_HANDLE) {
    vkDestroyCommandPool(device_, present_queue_.cmd_pool, nullptr);
    present_queue_.cmd_pool = VK_NULL_HANDLE;
  }
  if (graphics_queue_.cmd_pool != VK_NULL_HANDLE) {
    vkDestroyCommandPool(device_, graphics_queue_.cmd_pool, nullptr);
    graphics_queue_.cmd_pool = VK_NULL_HANDLE;
  }

  if (device_ != VK_NULL_HANDLE) {
    vkDeviceWaitIdle(device_);

    vkDestroyDevice(device_, nullptr);
    device_ = VK_NULL_HANDLE;
  }
}

uint32_t
VulkanDevice::GetMemoryType(uint32_t type_bits,
                            VkMemoryPropertyFlags properties_flags) const {
  const uint32_t &memory_type_count =
      physical_memory_properties_.memoryTypeCount;
  for (uint32_t i = 0U; i < memory_type_count; ++i) {
    if ((type_bits & (1U << i)) &&
        ((physical_memory_properties_.memoryTypes[i].propertyFlags &
          properties_flags) == properties_flags)) {
      return i;
    }
  }

  return -1;
}

bool VulkanDevice::IsPhysicalDeviceSuitable(VkPhysicalDevice physical_device,
                                            QueueFamilyIndices &queue_families,
                                            VkSurfaceKHR surface) const {
  std::ostringstream convertor;
  convertor << physical_device;

  // Check if the device supports the required extensions
  uint32_t extensions_count = 0U;
  if (vkEnumerateDeviceExtensionProperties(
          physical_device, nullptr, &extensions_count, nullptr) != VK_SUCCESS ||
      extensions_count == 0U) {
    ELOG_WARN("Physical device " + convertor.str() +
              " doesn't have extensions!");
    ELOG_FILE_WARN("Physical device " + convertor.str() +
                   " doesn't have extensions!");
    return false;
  }

  std::vector<VkExtensionProperties> available_extensions(extensions_count);
  if (vkEnumerateDeviceExtensionProperties(
          physical_device, nullptr, &extensions_count,
          available_extensions.data()) != VK_SUCCESS) {
    return false;
  }

  for (uint32_t i = 0; i < kDeviceExtensions.size(); ++i) {
    if (!tools::DoesPhysicalDeviceSupportExtension(kDeviceExtensions[i],
                                                   available_extensions)) {
      ELOG_WARN("Physical device " + convertor.str() +
                " doesn't support extension named \"" + kDeviceExtensions[i] +
                "\"!");
      ELOG_FILE_WARN("Physical device "
                     << convertor.str() +
                            " doesn't support extension named \"" +
                            kDeviceExtensions[i]
                     << "\"!");
      return false;
    }
  }

  // Check if the device supports the required properties and features
  VkPhysicalDeviceProperties device_properties;
  VkPhysicalDeviceFeatures device_features;

  vkGetPhysicalDeviceProperties(physical_device, &device_properties);
  vkGetPhysicalDeviceFeatures(physical_device, &device_features);

  uint32_t major_version = VK_VERSION_MAJOR(device_properties.apiVersion);
  // uint32_t minor_version = VK_VERSION_MINOR(device_properties.apiVersion);
  // uint32_t patch_version = VK_VERSION_PATCH(device_properties.apiVersion);

  if ((major_version < 1U) ||
      (device_properties.limits.maxImageDimension2D < 4096) ||
      (device_features.geometryShader == false)) {
    return false;
  }

  // Retrieve the number of queue families and enumerate them
  uint32_t queue_families_count = 0U;
  vkGetPhysicalDeviceQueueFamilyProperties(physical_device,
                                           &queue_families_count, nullptr);
  if (queue_families_count == 0U) {
    return false;
  }

  std::vector<VkQueueFamilyProperties> queue_family_properties(
      queue_families_count);
  vkGetPhysicalDeviceQueueFamilyProperties(
      physical_device, &queue_families_count, queue_family_properties.data());

  // Scan through the enumerated queue families and select a graphics,
  // compute and present queue; they could be on two separate families
  QueueFamilyIndices selected_queue_families = {UINT32_MAX, UINT32_MAX,
                                                UINT32_MAX};
  bool is_family_complete = false;
  for (uint32_t i = 0U; i < queue_families_count; ++i) {
    // Select the first queue familiy that supports graphics
    if ((queue_family_properties[i].queueCount > 0U) &&
        (queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
      selected_queue_families.graphics_family = i;
    }

    // Select a queue for presenting
    VkBool32 supports_present = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface,
                                         &supports_present);

    if ((queue_family_properties[i].queueCount > 0U) &&
        (supports_present == true)) {
      selected_queue_families.present_family = i;
    }

    // Select a compute queue
    if ((queue_family_properties[i].queueCount > 0U) &&
        (queue_family_properties[i].queueFlags & VK_QUEUE_COMPUTE_BIT)) {
      selected_queue_families.compute_family = i;
    }

    if (IsQueueFamilyIndicesComplete(selected_queue_families)) {
      is_family_complete = true;
      break;
    }
  }

  // If we couldn't find any suitable queues, this physical device cannot be
  // used
  if (!is_family_complete) {
    return false;
  }

  queue_families = selected_queue_families;
  return true;
}

void VulkanDevice::CreateImageView(
    const VkImageViewCreateInfo &image_vew_create_info,
    VulkanImage &image) const {

  VkImageView view = VK_NULL_HANDLE;
  VK_CHECK_RESULT(
      vkCreateImageView(device_, &image_vew_create_info, nullptr, &view));

  image.set_view(view);
}

bool IsQueueFamilyIndicesComplete(const QueueFamilyIndices &family_indices) {
  return (family_indices.graphics_family != UINT32_MAX &&
          family_indices.present_family != UINT32_MAX &&
          family_indices.compute_family != UINT32_MAX);
}

} // namespace vks
