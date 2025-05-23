#include <stdexcept>
#include "MemoryTypeFinder.hpp"

//--------------------------------------------------------------------------------------------------
MemoryTypeFinder::MemoryTypeFinder(VkPhysicalDevice device)
{
  this->physicalDevice = device;
}

//--------------------------------------------------------------------------------------------------
uint32_t MemoryTypeFinder::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
  VkPhysicalDeviceMemoryProperties memProperties;
  vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

  for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
  {
    if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
    {
      return i;
    }
  }

  throw std::runtime_error("failed to find suitable memory type!");
}
