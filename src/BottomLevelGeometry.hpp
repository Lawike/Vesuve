#pragma once
#include "VkTypes.hpp"

namespace VulkanBackend::Raytracing
{
  class BottomLevelGeometry
  {
   public:
    size_t size() const
    {
      return _geometry.size();
    }
    // Collection of geometries for the acceleration structure.
    const std::vector<VkAccelerationStructureGeometryKHR>& Geometry() const
    {
      return _geometry;
    }
    // Build range information corresponding to each geometry.
    const std::vector<VkAccelerationStructureBuildRangeInfoKHR>& BuildOffsetInfo() const
    {
      return _buildOffsetInfo;
    }
   private:
    // The geometry to build, addresses of vertices and indices.
    std::vector<VkAccelerationStructureGeometryKHR> _geometry;

    // the number of elements to build and offsets
    std::vector<VkAccelerationStructureBuildRangeInfoKHR> _buildOffsetInfo;
  };
}  // namespace VulkanBackend::Raytracing
