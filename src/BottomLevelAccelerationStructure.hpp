#pragma once
#include "AccelerationStructure.hpp"

namespace VulkanBackend::Raytracing
{
  class BottomLevelAccelerationStructure : public AccelerationStructure
  {
   public:
    BottomLevelAccelerationStructure(
      std::unique_ptr<Device>& device,
      std::unique_ptr<RaytracingProperties>& properties,
      std::vector<VkAccelerationStructureGeometryKHR>& geometries,
      std::vector<VkAccelerationStructureBuildRangeInfoKHR>& offsetInfo);

    void Generate(
      std::unique_ptr<Device>& device,
      VkCommandBuffer commandBuffer,
      AllocatedBuffer& scratchBuffer,
      const VkDeviceSize scratchOffset,
      AllocatedBuffer& resultBuffer,
      const VkDeviceSize resultOffset);
   private:
    std::vector<VkAccelerationStructureGeometryKHR> _geometry;
    // Build range information corresponding to each geometry.
    std::vector<VkAccelerationStructureBuildRangeInfoKHR> _offsetInfo;
  };
}  // namespace VulkanBackend::Raytracing
