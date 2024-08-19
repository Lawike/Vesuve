#include <algorithm>
#include "SwapChainSupportDetails.hpp"

//--------------------------------------------------------------------------------------------------
SwapChainSupportDetails::SwapChainSupportDetails(VkSurfaceKHR& surface)
{
  this->surface = surface;
}

//--------------------------------------------------------------------------------------------------
void SwapChainSupportDetails::querySwapChainSupport(VkPhysicalDevice& device)
{
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, this->surface, &(this->capabilities));

  uint32_t formatCount;
  vkGetPhysicalDeviceSurfaceFormatsKHR(device, this->surface, &formatCount, nullptr);

  if (formatCount != 0)
  {
    this->formats.resize(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, this->surface, &formatCount, this->formats.data());
  }

  uint32_t presentModeCount;
  vkGetPhysicalDeviceSurfacePresentModesKHR(device, this->surface, &presentModeCount, nullptr);

  if (presentModeCount != 0)
  {
    this->presentModes.resize(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, this->surface, &presentModeCount, this->presentModes.data());
  }
}

//--------------------------------------------------------------------------------------------------
VkSurfaceFormatKHR SwapChainSupportDetails::chooseSwapSurfaceFormat()
{
  for (const auto& availableFormat : this->formats)
  {
    if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
    {
      return availableFormat;
    }
  }

  return this->formats[0];
}

//--------------------------------------------------------------------------------------------------
VkPresentModeKHR SwapChainSupportDetails::chooseSwapPresentMode()
{
  for (const auto& availablePresentMode : this->presentModes)
  {
    if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
    {
      return availablePresentMode;
    }
  }

  return VK_PRESENT_MODE_FIFO_KHR;
}

//--------------------------------------------------------------------------------------------------
VkExtent2D SwapChainSupportDetails::chooseSwapExtent(GLFWwindow* window)
{
  if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
  {
    return capabilities.currentExtent;
  }
  else
  {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    VkExtent2D actualExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

    actualExtent.width =
      std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    actualExtent.height =
      std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    return actualExtent;
  }
}
