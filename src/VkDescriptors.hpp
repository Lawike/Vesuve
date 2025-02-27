#pragma once

#include <deque>
#include <span>
#include <vector>
#include "VkTypes.hpp"

struct DescriptorLayoutBuilder
{
  std::vector<VkDescriptorSetLayoutBinding> bindings;

  void addBinding(uint32_t binding, VkDescriptorType type, VkShaderStageFlags shaderStages, uint32_t descriptorCount);
  void clear();
  VkDescriptorSetLayout build(VkDevice device, void* pNext = nullptr, VkDescriptorSetLayoutCreateFlags flags = 0);
};

struct DescriptorWriter
{
  std::deque<VkDescriptorImageInfo> imageInfos;
  std::deque<VkDescriptorBufferInfo> bufferInfos;
  std::vector<VkWriteDescriptorSet> writes;

  void writeImage(int binding, VkImageView image, VkSampler sampler, VkImageLayout layout, VkDescriptorType type);
  void writeBuffer(int binding, VkBuffer buffer, size_t size, size_t offset, VkDescriptorType type);

  void clear();
  void updateSet(VkDevice device, VkDescriptorSet set);
};

struct DescriptorAllocator
{
  struct PoolSizeRatio
  {
    VkDescriptorType type;
    float ratio;
  };

  VkDescriptorPool pool;

  void initPool(VkDevice device, uint32_t maxSets, std::span<PoolSizeRatio> poolRatios);
  void clearDescriptors(VkDevice device);
  void destroyPool(VkDevice device);

  VkDescriptorSet allocate(VkDevice device, VkDescriptorSetLayout layout);
};

struct DescriptorAllocatorGrowable
{
 public:
  struct PoolSizeRatio
  {
    VkDescriptorType type;
    float ratio;
  };

  void init(VkDevice device, uint32_t maxSets, std::span<PoolSizeRatio> poolRatios);
  void clearPools(VkDevice device);
  void destroyPools(VkDevice device);

  VkDescriptorSet allocate(VkDevice device, VkDescriptorSetLayout layout, void* pNext = nullptr);
 private:
  VkDescriptorPool getPool(VkDevice device);
  VkDescriptorPool createPool(VkDevice device, uint32_t setCount, std::span<PoolSizeRatio> poolRatios);

  std::vector<PoolSizeRatio> ratios;
  std::vector<VkDescriptorPool> fullPools;
  std::vector<VkDescriptorPool> readyPools;
  uint32_t setsPerPool{1};
};

struct DescriptorBinding
{
  uint32_t binding;          // Slot to which the descriptor will be bound, corresponding to the layout index in the shader.
  uint32_t descriptorCount;  // Number of descriptors to bind
  VkDescriptorType type;     // Type of the bound descriptor(s)
  VkShaderStageFlags stage;  // Shader stage at which the bound resources will be available
};
