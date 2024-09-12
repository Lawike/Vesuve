#pragma once

#include <optional>
#include "VkTypes.hpp"


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
