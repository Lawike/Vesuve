#include <vector>
#include "QueueFamilyIndices.hpp"

//--------------------------------------------------------------------------------------------------
void QueueFamilyIndices::findQueueFamilies(VkPhysicalDevice& device, VkSurfaceKHR& surface)
{
  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

  int i = 0;
  for (const auto& queueFamily : queueFamilies)
  {
    if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
    {
      this->graphicsFamily = i;
    }

    VkBool32 presentSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

    if (presentSupport)
    {
      this->presentFamily = i;
    }

    if (this->isComplete())
    {
      break;
    }

    i++;
  }
}
