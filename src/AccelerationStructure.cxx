#include "AccelerationStructure.hpp"
#include "VkLoader.hpp"

//--------------------------------------------------------------------------------------------------
VulkanBackend::Raytracing::AccelerationStructure::AccelerationStructure()
{
  _flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
  _handle = VK_NULL_HANDLE;
}

//--------------------------------------------------------------------------------------------------
VkAccelerationStructureBuildSizesInfoKHR VulkanBackend::Raytracing::AccelerationStructure::getBuildSizes(
  std::unique_ptr<Device>& device,
  std::unique_ptr<RaytracingProperties>& properties,
  std::vector<uint32_t>& pMaxPrimitiveCounts)
{
  // Query both the size of the finished acceleration structure and the amount of scratch memory needed.
  VkAccelerationStructureBuildSizesInfoKHR sizeInfo = {};
  sizeInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
  auto getAccelerationStructureBuildSizesKHR = vkloader::loadFunction<PFN_vkGetAccelerationStructureBuildSizesKHR>(
    device->getHandle(), "vkGetAccelerationStructureBuildSizesKHR");
  getAccelerationStructureBuildSizesKHR(
    device->getHandle(),
    VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
    &_buildGeometryInfo,
    pMaxPrimitiveCounts.data(),
    &sizeInfo);

  // AccelerationStructure offset needs to be 256 bytes aligned (official Vulkan specs, don't ask me why).
  const uint64_t AccelerationStructureAlignment = 256;
  const uint64_t ScratchAlignment = properties->_accelProperties.minAccelerationStructureScratchOffsetAlignment;

  sizeInfo.accelerationStructureSize = RoundUp(sizeInfo.accelerationStructureSize, AccelerationStructureAlignment);
  sizeInfo.buildScratchSize = RoundUp(sizeInfo.buildScratchSize, ScratchAlignment);

  return sizeInfo;
}

//--------------------------------------------------------------------------------------------------
void VulkanBackend::Raytracing::AccelerationStructure::createAccelerationStructure(
  std::unique_ptr<Device>& device,
  AllocatedBuffer& resultBuffer,
  const VkDeviceSize resultOffset)
{
  VkAccelerationStructureCreateInfoKHR createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
  createInfo.pNext = nullptr;
  createInfo.type = _buildGeometryInfo.type;
  createInfo.size = _buildSizesInfo.accelerationStructureSize;
  createInfo.buffer = resultBuffer.buffer;
  createInfo.offset = resultOffset;

  auto createAccelerationStructureKHR =
    vkloader::loadFunction<PFN_vkCreateAccelerationStructureKHR>(device->getHandle(), "vkCreateAccelerationStructureKHR");
  VK_CHECK(createAccelerationStructureKHR(device->getHandle(), &createInfo, nullptr, &_handle));
}

//--------------------------------------------------------------------------------------------------
void VulkanBackend::Raytracing::AccelerationStructure::accelerationStructureBarrier(
  VkCommandBuffer cmd,
  VkAccessFlags src,
  VkAccessFlags dst)
{
  VkMemoryBarrier barrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
  barrier.srcAccessMask = src;
  barrier.dstAccessMask = dst;
  vkCmdPipelineBarrier(
    cmd,
    VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
    VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
    0,
    1,
    &barrier,
    0,
    nullptr,
    0,
    nullptr);
}

//--------------------------------------------------------------------------------------------------
uint64_t VulkanBackend::Raytracing::AccelerationStructure::RoundUp(uint64_t size, uint64_t granularity)
{
  const auto divUp = (size + granularity - 1) / granularity;
  return divUp * granularity;
}
