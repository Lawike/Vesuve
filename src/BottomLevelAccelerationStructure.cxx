#include "BottomLevelAccelerationStructure.hpp"
#include "VkLoader.hpp"

//--------------------------------------------------------------------------------------------------
VulkanBackend::Raytracing::BottomLevelAccelerationStructure::BottomLevelAccelerationStructure(
  std::unique_ptr<Device>& device,
  std::unique_ptr<RaytracingProperties>& properties,
  std::vector<VkAccelerationStructureGeometryKHR> geometry,
  std::vector<VkAccelerationStructureBuildRangeInfoKHR> offsetInfo)
    : AccelerationStructure()
{
  _geometry = geometry;
  _offsetInfo = offsetInfo;
  _buildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
  _buildGeometryInfo.flags = _flags;
  _buildGeometryInfo.geometryCount = static_cast<uint32_t>(_geometry.size());
  _buildGeometryInfo.pGeometries = _geometry.data();
  _buildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
  _buildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
  _buildGeometryInfo.srcAccelerationStructure = nullptr;

  std::vector<uint32_t> maxPrimCount(_offsetInfo.size());

  for (size_t i = 0; i != maxPrimCount.size(); ++i)
  {
    maxPrimCount[i] = _offsetInfo[i].primitiveCount;
  }

  _buildSizesInfo = getBuildSizes(device, properties, maxPrimCount.data());
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
  _buildGeometryInfo.dstAccelerationStructure = _handle;
  VkBufferDeviceAddressInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
  info.pNext = nullptr;
  info.buffer = scratchBuffer.buffer;
  VkDeviceAddress scratchBufferAdress = vkGetBufferDeviceAddress(device->getHandle(), &info);
  _buildGeometryInfo.scratchData.deviceAddress = scratchBufferAdress + scratchOffset;
  auto cmdBuildAccelerationStructuresKHR = vkloader::loadFunction<PFN_vkCmdBuildAccelerationStructuresKHR>(
    device->getHandle(), "vkCmdBuildAccelerationStructuresKHR");
  cmdBuildAccelerationStructuresKHR(commandBuffer, 1, &_buildGeometryInfo, &pBuildOffsetInfo);
}
