#include "Device.hpp"

//--------------------------------------------------------------------------------------------------
VulkanBackend::Device::Device(std::unique_ptr<PhysicalDevice>& physicalDevice)
{
  //create the final vulkan device
  //vkBoostrap Provides a single queue of each queue family by default
  vkb::DeviceBuilder deviceBuilder{physicalDevice->getHandle()};

  _vkbHandle = deviceBuilder.build().value();

  // use vkbootstrap to get a Graphics queue
  _graphicsQueue = _vkbHandle.get_queue(vkb::QueueType::graphics).value();
  _graphicsQueueFamily = _vkbHandle.get_queue_index(vkb::QueueType::graphics).value();
}
