#pragma once
#include "Device.hpp"
#include "VkTypes.hpp"


namespace VulkanBackend
{
  class Semaphore
  {
   public:
    Semaphore(std::unique_ptr<Device>& Device);

    VkSemaphore _handle;
  };
}  // namespace VulkanBackend
