#pragma once
#include "PhysicalDevice.hpp"

namespace VulkanBackend
{
  class Device
  {
   public:
    Device(std::unique_ptr<PhysicalDevice>& physicalDevice);

    vkb::Device getHandle() const
    {
      return _vkbHandle;
    }

    VkQueue getGraphicsQueue() const
    {
      return _graphicsQueue;
    }

    uint32_t getGraphicsQueueFamily() const
    {
      return _graphicsQueueFamily;
    }

   private:
    vkb::Device _vkbHandle;
    VkQueue _graphicsQueue;
    uint32_t _graphicsQueueFamily;
  };

}  // namespace VulkanBackend
