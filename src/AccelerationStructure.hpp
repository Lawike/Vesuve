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
    // Build information required for acceleration structure.
    VkAccelerationStructureBuildGeometryInfoKHR _buildGeometryInfo{};
    // Size information for acceleration structure build resources.
    VkAccelerationStructureBuildSizesInfoKHR _buildSizesInfo{};
    // Helper function to insert a memory barrier for acceleration structures
    void accelerationStructureBarrier(VkCommandBuffer cmd, VkAccessFlags src, VkAccessFlags dst);
   protected:
    AccelerationStructure();
    VkAccelerationStructureBuildSizesInfoKHR getBuildSizes(
      std::unique_ptr<Device>& device,
      std::unique_ptr<RaytracingProperties>& properties,
      std::vector<uint32_t>& pMaxPrimitiveCounts);
    void createAccelerationStructure(
      std::unique_ptr<Device>& device,
      AllocatedBuffer& resultBuffer,
      const VkDeviceSize resultOffset);
    VkBuildAccelerationStructureFlagsKHR _flags;

   private:
    uint64_t RoundUp(uint64_t size, uint64_t granularity);
  };
}  // namespace VulkanBackend::Raytracing
