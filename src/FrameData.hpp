#pragma once
#include "CommandBuffer.hpp"
#include "CommandPool.hpp"
#include "Device.hpp"
#include "Fence.hpp"
#include "Semaphore.hpp"
#include "VkDescriptors.hpp"
#include "VkTypes.hpp"

namespace VulkanBackend
{
  class FrameData
  {
   public:
    FrameData(std::unique_ptr<Device>& device);

    DeletionQueue _deletionQueue;
    DescriptorAllocatorGrowable _frameDescriptors;

    std::unique_ptr<CommandPool> _commandPool;
    std::unique_ptr<CommandBuffer> _mainCommandBuffer;
    std::unique_ptr<Fence> _renderFence;
    std::unique_ptr<Semaphore> _swapchainSemaphore, _renderSemaphore;
  };
}  // namespace VulkanBackend
