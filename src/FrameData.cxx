#include "FrameData.hpp"
#include "VkInitializers.hpp"

//--------------------------------------------------------------------------------------------------
VulkanBackend::FrameData::FrameData(std::unique_ptr<Device>& device)
{
  _commandPool = std::make_unique<CommandPool>(device);
  _mainCommandBuffer = std::make_unique<CommandBuffer>(device, _commandPool);
  _renderFence = std::make_unique<Fence>(device);
  _swapchainSemaphore = std::make_unique<Semaphore>(device);
  _renderSemaphore = std::make_unique<Semaphore>(device);

  // create a descriptor pool
  std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> frame_sizes = {
    {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3},
    {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3},
    {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3},
    {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4},
  };

  _frameDescriptors = DescriptorAllocatorGrowable{};
  _frameDescriptors.init(device->getHandle(), 1000, frame_sizes);
}
