#pragma once

#include "Device.hpp"
#include "VkTypes.hpp"

namespace VulkanBackend
{
  class Fence
  {
   public:
    Fence(std::unique_ptr<Device>& Device);

    VkFence _handle;
  };
}  // namespace VulkanBackend
