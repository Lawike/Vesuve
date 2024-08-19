#include <optional>
#include <set>
#include <stdexcept>
#include "PhysicalDevicePicker.hpp"
#include "QueueFamilyIndices.hpp"
#include "SwapChainSupportDetails.hpp"

//--------------------------------------------------------------------------------------------------
PhysicalDevicePicker::PhysicalDevicePicker(VkSurfaceKHR& surface, std::vector<const char*>& deviceExtensions)
{
  this->surface = surface;
  this->deviceExtensions = deviceExtensions;
}

//--------------------------------------------------------------------------------------------------
void PhysicalDevicePicker::pick(std::vector<VkPhysicalDevice> devices)
{
  for (const auto& device : devices)
  {
    if (isDeviceSuitable(device))
    {
      this->pickedDevice = device;
      this->msaaSamples = getMaxUsableSampleCount(pickedDevice);
      break;
    }
  }

  if (this->pickedDevice == VK_NULL_HANDLE)
  {
    throw std::runtime_error("failed to find a suitable GPU!");
  }
}

//--------------------------------------------------------------------------------------------------
bool PhysicalDevicePicker::isDeviceSuitable(VkPhysicalDevice device)
{
  QueueFamilyIndices indices;
  indices.findQueueFamilies(device, this->surface);

  bool extensionsSupported = checkDeviceExtensionSupport(device);

  bool swapChainAdequate = false;
  if (extensionsSupported)
  {
    SwapChainSupportDetails swapChainSupport{surface};
    swapChainSupport.querySwapChainSupport(device);
    swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
  }

  VkPhysicalDeviceFeatures supportedFeatures;
  vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

  return indices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
};

//--------------------------------------------------------------------------------------------------
VkSampleCountFlagBits PhysicalDevicePicker::getMaxUsableSampleCount(VkPhysicalDevice& device)
{
  VkPhysicalDeviceProperties physicalDeviceProperties;
  vkGetPhysicalDeviceProperties(device, &physicalDeviceProperties);

  VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts &
                              physicalDeviceProperties.limits.framebufferDepthSampleCounts;
  if (counts & VK_SAMPLE_COUNT_64_BIT)
  {
    return VK_SAMPLE_COUNT_64_BIT;
  }
  if (counts & VK_SAMPLE_COUNT_32_BIT)
  {
    return VK_SAMPLE_COUNT_32_BIT;
  }
  if (counts & VK_SAMPLE_COUNT_16_BIT)
  {
    return VK_SAMPLE_COUNT_16_BIT;
  }
  if (counts & VK_SAMPLE_COUNT_8_BIT)
  {
    return VK_SAMPLE_COUNT_8_BIT;
  }
  if (counts & VK_SAMPLE_COUNT_4_BIT)
  {
    return VK_SAMPLE_COUNT_4_BIT;
  }
  if (counts & VK_SAMPLE_COUNT_2_BIT)
  {
    return VK_SAMPLE_COUNT_2_BIT;
  }

  return VK_SAMPLE_COUNT_1_BIT;
};

//--------------------------------------------------------------------------------------------------
bool PhysicalDevicePicker::checkDeviceExtensionSupport(VkPhysicalDevice& device)
{
  uint32_t extensionCount;
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

  std::vector<VkExtensionProperties> availableExtensions(extensionCount);
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

  std::set<std::string> requiredExtensions(this->deviceExtensions.begin(), this->deviceExtensions.end());

  for (const auto& extension : availableExtensions)
  {
    requiredExtensions.erase(extension.extensionName);
  }

  return requiredExtensions.empty();
}
