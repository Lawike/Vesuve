#include "TopLevelAccelerationStructure.hpp"
#include "VkLoader.hpp"

//--------------------------------------------------------------------------------------------------
VulkanBackend::Raytracing::TopLevelAccelerationStructure::TopLevelAccelerationStructure(
  std::unique_ptr<Device>& device,
  std::unique_ptr<RaytracingProperties>& properties,
  AllocatedBuffer& resultBuffer,
  const VkDeviceSize resultOffset,
  VkDeviceAddress instanceAddress,
  uint32_t instancesCount)
    : AccelerationStructure()
{
  _instancesCount = instancesCount;
  // Create VkAccelerationStructureGeometryInstancesDataKHR. This wraps a device pointer to the uploaded instances.
  _instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
  _instances.arrayOfPointers = VK_FALSE;
  _instances.data.deviceAddress = instanceAddress;

  // Put the above into a VkAccelerationStructureGeometryKHR. We need to put the
  // instances struct in a union and label it as instance data.
  _topASGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
  _topASGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
  _topASGeometry.geometry.instances = _instances;

  _buildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
  _buildGeometryInfo.flags = _flags;
  _buildGeometryInfo.geometryCount = 1;
  _buildGeometryInfo.pGeometries = &_topASGeometry;
  _buildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
  _buildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
  _buildGeometryInfo.srcAccelerationStructure = nullptr;

  _buildSizesInfo = getBuildSizes(device, properties, &instancesCount);
}

//--------------------------------------------------------------------------------------------------
void VulkanBackend::Raytracing::TopLevelAccelerationStructure::Generate(
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
  VkAccelerationStructureBuildRangeInfoKHR buildOffsetInfo = {};
  buildOffsetInfo.primitiveCount = _instancesCount;

  const VkAccelerationStructureBuildRangeInfoKHR* pBuildOffsetInfo = &buildOffsetInfo;

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

//--------------------------------------------------------------------------------------------------
VkAccelerationStructureInstanceKHR VulkanBackend::Raytracing::TopLevelAccelerationStructure::CreateInstance(
  std::unique_ptr<Device>& device,
  BottomLevelAccelerationStructure& bottomLevelAs,
  const glm::mat4& transform,
  const uint32_t instanceId,
  const uint32_t hitGroupId)
{
  VkAccelerationStructureDeviceAddressInfoKHR addressInfo = {};
  addressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
  addressInfo.accelerationStructure = bottomLevelAs._handle;
  auto getAccelerationStructureDeviceAddressKHR = vkloader::loadFunction<PFN_vkGetAccelerationStructureDeviceAddressKHR>(
    device->getHandle(), "vkGetAccelerationStructureDeviceAddressKHR");
  const VkDeviceAddress address = getAccelerationStructureDeviceAddressKHR(device->getHandle(), &addressInfo);

  VkAccelerationStructureInstanceKHR instance = {};
  instance.instanceCustomIndex = instanceId;
  instance.mask =
    0xFF;  // The visibility mask is always set of 0xFF, but if some instances would need to be ignored in some cases, this flag should be passed by the application.
  instance.instanceShaderBindingTableRecordOffset =
    hitGroupId;  // Set the hit group index, that will be used to find the shader code to execute when hitting the geometry.
  instance.flags =
    VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;  // Disable culling - more fine control could be provided by the application
  instance.accelerationStructureReference = address;

  // The instance.transform value only contains 12 values, corresponding to a 4x3 matrix,
  // hence saving the last row that is anyway always (0,0,0,1).
  // Since the matrix is row-major, we simply copy the first 12 values of the original 4x4 matrix
  std::memcpy(&instance.transform, &transform, sizeof(instance.transform));

  return instance;
}
