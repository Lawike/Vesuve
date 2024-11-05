#pragma once
#include "Device.hpp"
#include "VkTypes.hpp"

namespace VulkanBackend
{
  class CommandPool
  {
   public:
    CommandPool(std::unique_ptr<Device>& device);

    VkCommandPool getHandle() const
    {
      return _handle;
    }
   private:
    VkCommandPool _handle;
  };

}  // namespace VulkanBackend
