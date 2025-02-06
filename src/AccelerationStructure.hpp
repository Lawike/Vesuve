#pragma once

#include "Device.hpp"
#include "RaytracingProperties.hpp"
#include "VkTypes.hpp"

namespace VulkanBackend::Raytracing
{
  class AccelerationStructure
  {
   public:
    VkAccelerationStructureKHR _handle;
    VkAccelerationStructureBuildGeometryInfoKHR _buildGeometryInfo{};
    VkAccelerationStructureBuildSizesInfoKHR _buildSizesInfo{};
   protected:
    AccelerationStructure();
    VkAccelerationStructureBuildSizesInfoKHR getBuildSizes(
      std::unique_ptr<Device>& device,
      std::unique_ptr<RaytracingProperties>& properties,
      const uint32_t* pMaxPrimitiveCounts);
    void createAccelerationStructure(
      std::unique_ptr<Device>& device,
      AllocatedBuffer& resultBuffer,
      const VkDeviceSize resultOffset);
    VkBuildAccelerationStructureFlagsKHR _flags;

   private:
    uint64_t RoundUp(uint64_t size, uint64_t granularity);
  };
}  // namespace VulkanBackend::Raytracing
