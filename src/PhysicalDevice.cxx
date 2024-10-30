#include "PhysicalDevice.hpp"

//--------------------------------------------------------------------------------------------------
VulkanBackend::PhysicalDevice::PhysicalDevice(std::unique_ptr<Instance>& instance, VkSurfaceKHR& surface)
{
  //vulkan 1.3 features
  _features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
  _features13.dynamicRendering = true;
  _features13.synchronization2 = true;

  //vulkan 1.2 features
  _features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
  _features12.bufferDeviceAddress = true;
  _features12.descriptorIndexing = true;


  //use vkbootstrap to select a gpu.
  //We want a gpu that can write to the SDL surface and supports vulkan 1.3 with the correct features
  vkb::Instance vkbInstanceHandle = instance->getHandle();
  vkb::PhysicalDeviceSelector selector{vkbInstanceHandle};
  _vkbHandle = selector.set_minimum_version(1, 3)
                 .set_required_features_13(_features13)
                 .set_required_features_12(_features12)
                 .set_surface(surface)
                 .select()
                 .value();
}
