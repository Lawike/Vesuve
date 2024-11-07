#pragma once

#include "Device.hpp"

namespace VulkanBackend
{
  class Image
  {
   public:
    Image(
      std::unique_ptr<Device>& device,
      VkExtent3D imageExtent,
      VkFormat imageFormat,
      VkImageUsageFlags usage,
      VmaAllocator allocator,
      bool mipmapped);
    ~Image() = default;

    AllocatedImage _handle{};
  };
}  // namespace VulkanBackend
