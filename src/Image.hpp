#pragma once

#include "VkTypes.hpp"

namespace VulkanBackend
{
  class Image
  {
   public:
    Image(VkExtent2D _windowExtent, VkFormat imageFormat, VkImageUsageFlags usage, VmaAllocator allocator);

    AllocatedImage _handle{};
   private:
  };
}  // namespace VulkanBackend
