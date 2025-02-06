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
  // Required for Raytracing
  _features12.bufferDeviceAddress = true;
  _features12.descriptorIndexing = true;

  VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures = {};
  accelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
  accelerationStructureFeatures.pNext = nullptr;
  accelerationStructureFeatures.accelerationStructure = true;

  VkPhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingFeatures = {};
  rayTracingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
  rayTracingFeatures.pNext = nullptr;
  rayTracingFeatures.rayTracingPipeline = true;

  //use vkbootstrap to select a gpu.
  //We want a gpu that can write to the SDL surface and supports vulkan 1.3 with the correct features
  vkb::Instance vkbInstanceHandle = instance->getHandle();
  vkb::PhysicalDeviceSelector selector{vkbInstanceHandle};
  _vkbHandle = selector.set_minimum_version(1, 3)
                 .set_required_features_13(_features13)
                 .set_required_features_12(_features12)
                 .add_required_extension(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME)
                 .add_required_extension(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME)
                 .add_required_extension(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME)
                 .add_required_extension_features(accelerationStructureFeatures)
                 .add_required_extension_features(rayTracingFeatures)
                 .set_surface(surface)
                 .select()
                 .value();
}
