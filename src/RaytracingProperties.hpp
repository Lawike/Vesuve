#pragma once

#include "Device.hpp"
#include "VkTypes.hpp"

namespace VulkanBackend
{
  namespace Raytracing
  {
    class RaytracingProperties
    {
     public:
      RaytracingProperties(const std::unique_ptr<PhysicalDevice>& physicalDevice);

      VkPhysicalDeviceAccelerationStructurePropertiesKHR _accelProperties{};
      VkPhysicalDeviceRayTracingPipelinePropertiesKHR _pipelineProperties{};
     private:
    };
  }  // namespace Raytracing
}  // namespace VulkanBackend
