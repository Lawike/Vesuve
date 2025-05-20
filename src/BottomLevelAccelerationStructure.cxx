#include "BottomLevelAccelerationStructure.hpp"
#include "VkLoader.hpp"

//--------------------------------------------------------------------------------------------------
VulkanBackend::Raytracing::BottomLevelAccelerationStructure::BottomLevelAccelerationStructure(
  std::unique_ptr<Device>& device,
  std::unique_ptr<RaytracingProperties>& properties,
  std::vector<VkAccelerationStructureGeometryKHR>& geometries,
  std::vector<VkAccelerationStructureBuildRangeInfoKHR>& offsetInfo)
    : AccelerationStructure()
{
  _geometry = geometries;
  assert(_geometry.size() > 0 && "No geometry added to Build Structure");

  _offsetInfo = offsetInfo;
  _buildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
  _buildGeometryInfo.flags =
    _flags | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_DATA_ACCESS_KHR;  // For Ray Tracing Position Fetch extension
  _buildGeometryInfo.geometryCount = static_cast<uint32_t>(_geometry.size());
  _buildGeometryInfo.pGeometries = _geometry.data();
  _buildGeometryInfo.ppGeometries = nullptr;
  _buildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
  _buildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
  _buildGeometryInfo.srcAccelerationStructure = VK_NULL_HANDLE;
  _buildGeometryInfo.scratchData.deviceAddress = 0;

  std::vector<uint32_t> maxPrimCount(_offsetInfo.size());

  for (size_t i = 0; i < maxPrimCount.size(); ++i)
  {
    maxPrimCount[i] = _offsetInfo[i].primitiveCount;
  }

  _buildSizesInfo = getBuildSizes(device, properties, maxPrimCount);
}

//--------------------------------------------------------------------------------------------------
void VulkanBackend::Raytracing::BottomLevelAccelerationStructure::Generate(
  std::unique_ptr<Device>& device,
  VkCommandBuffer commandBuffer,
  AllocatedBuffer& scratchBuffer,
  const VkDeviceSize scratchOffset,
  AllocatedBuffer& resultBuffer,
  const VkDeviceSize resultOffset)
{
  // Create the acceleration structure.
  createAccelerationStructure(device, resultBuffer, resultOffset);

  // Build the actual bottom-level acceleration structure
  const VkAccelerationStructureBuildRangeInfoKHR* pBuildOffsetInfo = _offsetInfo.data();
  VkBufferDeviceAddressInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
  info.pNext = nullptr;
  info.buffer = scratchBuffer.buffer;
  VkDeviceAddress scratchBufferAdress = vkGetBufferDeviceAddress(device->getHandle(), &info);
  _buildGeometryInfo.scratchData.deviceAddress = scratchBufferAdress + scratchOffset;
  _buildGeometryInfo.dstAccelerationStructure = _handle;

  auto cmdBuildAccelerationStructuresKHR = vkloader::loadFunction<PFN_vkCmdBuildAccelerationStructuresKHR>(
    device->getHandle(), "vkCmdBuildAccelerationStructuresKHR");
  cmdBuildAccelerationStructuresKHR(commandBuffer, 1, &_buildGeometryInfo, &pBuildOffsetInfo);

  // Barrier to ensure proper synchronization after building
  accelerationStructureBarrier(
    commandBuffer,
    VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR,
    VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR | VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR);
}
