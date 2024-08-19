#pragma once

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif

#include <optional>


class QueueFamilyIndices
{
 public:
  std::optional<uint32_t> graphicsFamily;
  std::optional<uint32_t> presentFamily;

  QueueFamilyIndices() = default;
  ~QueueFamilyIndices() = default;

  bool isComplete()
  {
    return graphicsFamily.has_value() && presentFamily.has_value();
  }

  void findQueueFamilies(VkPhysicalDevice& device, VkSurfaceKHR& surface);
};
