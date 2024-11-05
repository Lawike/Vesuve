#pragma once
#include "CommandPool.hpp"
#include "Device.hpp"
#include "VkTypes.hpp"

namespace VulkanBackend
{
  class CommandBuffer
  {
   public:
    CommandBuffer(std::unique_ptr<Device>& device, std::unique_ptr<CommandPool>& commandPool);
    VkCommandBuffer getHandle() const
    {
      return _handle;
    }
   private:
    VkCommandBuffer _handle;
  };
}  // namespace VulkanBackend
