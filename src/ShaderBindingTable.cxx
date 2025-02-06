#include "ShaderBindingTable.hpp"
#include "VkLoader.hpp"

//--------------------------------------------------------------------------------------------------
VulkanBackend::Raytracing::ShaderBindingTable::ShaderBindingTable(
  const std::unique_ptr<Device>& device,
  const VmaAllocator& allocator,
  const std::unique_ptr<RaytracingProperties>& properties,
  const std::unique_ptr<RaytracingPipeline>& rayTracingPipeline,
  const std::vector<Entry>& rayGenPrograms,
  const std::vector<Entry>& missPrograms,
  const std::vector<Entry>& hitGroups)
{
  _raygenEntrySize = getEntrySize(rayGenPrograms, properties);
  _missEntrySize = getEntrySize(missPrograms, properties);
  _hitGroupEntrySize = getEntrySize(hitGroups, properties);

  _raygenOffset = 0;
  _missOffset = rayGenPrograms.size() + _raygenEntrySize;
  _hitGroupOffset = _missOffset + missPrograms.size() * _missEntrySize;

  _raygenSize = rayGenPrograms.size() * _raygenEntrySize;
  _missSize = missPrograms.size() * _missEntrySize;
  _hitGroupSize = hitGroups.size() * _hitGroupEntrySize;

  // Compute the size of the table.
  const size_t shaderBindingTableSize = _raygenSize + _missSize + _hitGroupSize;

  // Allocate buffer & memory.
  VkBufferCreateInfo bufferInfo = {.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  bufferInfo.pNext = nullptr;
  bufferInfo.size = shaderBindingTableSize;
  bufferInfo.usage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                     VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR;

  VmaAllocationCreateInfo vmaallocInfo = {};
  vmaallocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
  vmaallocInfo.flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

  VK_CHECK(vmaCreateBuffer(allocator, &bufferInfo, &vmaallocInfo, &_handle.buffer, &_handle.allocation, &_handle.info));

  // Generate the table.
  const uint32_t handleSize = properties->_pipelineProperties.shaderGroupHandleSize;
  const size_t groupCount = rayGenPrograms.size() + missPrograms.size() + hitGroups.size();
  std::vector<uint8_t> shaderHandleStorage(groupCount * handleSize);

  auto getRayTracingShaderGroupHandlesKHR = vkloader::loadFunction<PFN_vkGetRayTracingShaderGroupHandlesKHR>(
    device->getHandle(), "vkGetRayTracingShaderGroupHandlesKHR");

  VK_CHECK(getRayTracingShaderGroupHandlesKHR(
    device->getHandle(),
    rayTracingPipeline->_handle,
    0,
    static_cast<uint32_t>(groupCount),
    shaderHandleStorage.size(),
    shaderHandleStorage.data()));

  // Copy the shader identifiers followed by their resource pointers or root constants:
  // first the ray generation, then the miss shaders, and finally the set of hit groups.
  void* pData;
  VK_CHECK(vmaMapMemory(allocator, _handle.allocation, &pData));

  uint8_t* shaderTableData = static_cast<uint8_t*>(pData);

  shaderTableData +=
    copyShaderData(shaderTableData, properties, rayGenPrograms, _raygenEntrySize, shaderHandleStorage.data());
  shaderTableData += copyShaderData(shaderTableData, properties, missPrograms, _missEntrySize, shaderHandleStorage.data());
  copyShaderData(shaderTableData, properties, hitGroups, _hitGroupEntrySize, shaderHandleStorage.data());

  vmaUnmapMemory(allocator, _handle.allocation);
}

//--------------------------------------------------------------------------------------------------
size_t VulkanBackend::Raytracing::ShaderBindingTable::getEntrySize(
  const std::vector<ShaderBindingTable::Entry>& entries,
  const std::unique_ptr<RaytracingProperties>& rayTracingProperties)
{
  // Find the maximum number of parameters used by a single entry
  size_t maxArgs = 0;

  for (const auto& entry : entries)
  {
    maxArgs = std::max(maxArgs, entry.InlineData.size());
  }

  // A SBT entry is made of a program ID and a set of 4-byte parameters (see shaderRecordEXT).
  // Its size is ShaderGroupHandleSize (plus parameters) and must be aligned to ShaderGroupBaseAlignment.
  return roundUp(
    rayTracingProperties->_pipelineProperties.shaderGroupHandleSize + maxArgs,
    rayTracingProperties->_pipelineProperties.shaderGroupBaseAlignment);
}

//--------------------------------------------------------------------------------------------------
size_t VulkanBackend::Raytracing::ShaderBindingTable::roundUp(size_t size, size_t powerOf2Alignment)
{
  return (size + powerOf2Alignment - 1) & ~(powerOf2Alignment - 1);
}

//--------------------------------------------------------------------------------------------------
size_t VulkanBackend::Raytracing::ShaderBindingTable::copyShaderData(
  uint8_t* const dst,
  const std::unique_ptr<RaytracingProperties>& properties,
  const std::vector<ShaderBindingTable::Entry>& entries,
  const size_t entrySize,
  const uint8_t* const shaderHandleStorage)
{
  const auto handleSize = properties->_pipelineProperties.shaderGroupHandleSize;

  uint8_t* pDst = dst;

  for (const auto& entry : entries)
  {
    // Copy the shader identifier that was previously obtained with vkGetRayTracingShaderGroupHandlesKHR.
    std::memcpy(pDst, shaderHandleStorage + entry.GroupIndex * handleSize, handleSize);
    std::memcpy(pDst + handleSize, entry.InlineData.data(), entry.InlineData.size());

    pDst += entrySize;
  }

  return entries.size() * entrySize;
}
