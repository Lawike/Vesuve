#pragma once

#include "VkTypes.hpp"

class Buffer
{
 public:
  VkDeviceSize size;
  VkBufferUsageFlags usage;
  VkMemoryPropertyFlags properties;
  VkBuffer refBuffer{};
  VkDeviceMemory memory{};

  Buffer(
    VkPhysicalDevice physicalDevice,
    VkDevice device,
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties);
  void copy(Buffer srcBuffer, VkDeviceSize size, VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue);

 private:
  VkPhysicalDevice physicalDevice;
  VkDevice device;

  void createBuffer();
};
