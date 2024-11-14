#pragma once
#include "Device.hpp"
#include "RaytracingPipeline.hpp"
#include "RaytracingProperties.hpp"
#include "VkTypes.hpp"

namespace VulkanBackend
{
  namespace Raytracing
  {
    class ShaderBindingTable
    {
     public:
      struct Entry
      {
        uint32_t GroupIndex;
        std::vector<unsigned char> InlineData;
      };

      ShaderBindingTable(
        const std::unique_ptr<Device>& device,
        const VmaAllocator& allocator,
        const std::unique_ptr<RaytracingProperties>& properties,
        const std::unique_ptr<RaytracingPipeline>& rayTracingPipeline,
        const std::vector<Entry>& rayGenPrograms,
        const std::vector<Entry>& missPrograms,
        const std::vector<Entry>& hitGroups);

      AllocatedBuffer _handle;
      VkDeviceAddress _raygenShaderAddress;
      VkDeviceAddress _missShaderAddress;
      VkDeviceAddress _closesHitShaderAddress;
      VkDeviceAddress _proceduralClosestHitShaderAddress;
      VkDeviceAddress _proceduralIntersectionShaderAddress;

      size_t _raygenOffset;
      size_t _missOffset;
      size_t _hitGroupOffset;

      size_t _raygenSize;
      size_t _missSize;
      size_t _hitGroupSize;

      size_t _raygenEntrySize;
      size_t _missEntrySize;
      size_t _hitGroupEntrySize;

     private:
      size_t getEntrySize(
        const std::vector<ShaderBindingTable::Entry>& entries,
        const std::unique_ptr<RaytracingProperties>& rayTracingProperties);
      size_t roundUp(size_t size, size_t powerOf2Alignment);
      size_t copyShaderData(
        uint8_t* const dst,
        const std::unique_ptr<RaytracingProperties>& rayTracingProperties,
        const std::vector<ShaderBindingTable::Entry>& entries,
        const size_t entrySize,
        const uint8_t* const shaderHandleStorage);
    };
  }  // namespace Raytracing
}  // namespace VulkanBackend
