#include "PhysicalDevice.hpp"

//--------------------------------------------------------------------------------------------------
VulkanBackend::PhysicalDevice::PhysicalDevice(std::unique_ptr<Instance>& instance, VkSurfaceKHR& surface)
{
  //vulkan 1.3 features
  _features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
  _features13.dynamicRendering = true;
  _features13.synchronization2 = true;
  _features13.maintenance4 = true;

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

  // Enable fence for each swapchain image sempahore
  VkPhysicalDeviceSwapchainMaintenance1FeaturesEXT swapChainMaintenanceFeatures = {};
  swapChainMaintenanceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SWAPCHAIN_MAINTENANCE_1_FEATURES_EXT;
  swapChainMaintenanceFeatures.swapchainMaintenance1 = true;

  // Enable RT position fetch to avoid unpacking vertex
  VkPhysicalDeviceRayTracingPositionFetchFeaturesKHR RTPositionFetchFeatures{};
  RTPositionFetchFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_POSITION_FETCH_FEATURES_KHR;
  RTPositionFetchFeatures.rayTracingPositionFetch = true;

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
                 .add_required_extension(VK_KHR_SPIRV_1_4_EXTENSION_NAME)
                 .add_required_extension(VK_NV_RAY_TRACING_VALIDATION_EXTENSION_NAME)
                 .add_required_extension(VK_EXT_SWAPCHAIN_MAINTENANCE_1_EXTENSION_NAME)
                 .add_required_extension(VK_KHR_RAY_TRACING_POSITION_FETCH_EXTENSION_NAME)
                 .add_required_extension_features(accelerationStructureFeatures)
                 .add_required_extension_features(rayTracingFeatures)
                 .add_required_extension_features(swapChainMaintenanceFeatures)
                 .add_required_extension_features(RTPositionFetchFeatures)
                 .set_surface(surface)
                 .select()
                 .value();
}
