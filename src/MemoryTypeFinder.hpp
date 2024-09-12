#pragma once

#include "VkTypes.hpp"

class MemoryTypeFinder
{
 public:
  VkPhysicalDevice physicalDevice;

  MemoryTypeFinder(VkPhysicalDevice device);

  uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
};
