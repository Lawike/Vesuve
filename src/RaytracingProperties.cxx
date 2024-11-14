#include "RaytracingProperties.hpp"

VulkanBackend::Raytracing::RaytracingProperties::RaytracingProperties(const std::unique_ptr<PhysicalDevice>& physicalDevice)
{
  _accelProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR;
  _pipelineProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
  _pipelineProperties.pNext = &_accelProperties;

  VkPhysicalDeviceProperties2 props = {};
  props.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
  props.pNext = &_pipelineProperties;
  vkGetPhysicalDeviceProperties2(physicalDevice->getHandle(), &props);
}
