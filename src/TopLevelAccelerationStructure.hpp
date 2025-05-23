#pragma once

#include "BottomLevelAccelerationStructure.hpp"

namespace VulkanBackend::Raytracing
{
  class TopLevelAccelerationStructure : public AccelerationStructure
  {
   public:
    TopLevelAccelerationStructure(
      std::unique_ptr<Device>& device,
      std::unique_ptr<RaytracingProperties>& properties,
      AllocatedBuffer& resultBuffer,
      const VkDeviceSize resultOffset,
      VkDeviceAddress instanceAddress,
      uint32_t instancesCount);
    void Generate(
      std::unique_ptr<Device>& device,
      VkCommandBuffer commandBuffer,
      AllocatedBuffer& scratchBuffer,
      const VkDeviceSize scratchOffset,
      AllocatedBuffer& resultBuffer,
      const VkDeviceSize resultOffset);
    static VkAccelerationStructureInstanceKHR CreateInstance(
      std::unique_ptr<Device>& device,
      BottomLevelAccelerationStructure& bottomLevelAs,
      const glm::mat4& transform,
      const uint32_t instanceId,
      const uint32_t hitGroupId,
      const uint32_t mask);
   private:
    uint32_t _instancesCount;
    VkAccelerationStructureGeometryInstancesDataKHR _instances{};
    VkAccelerationStructureGeometryKHR _topASGeometry{};
  };
}  // namespace VulkanBackend::Raytracing
