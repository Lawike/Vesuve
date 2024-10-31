#include "Device.hpp"
#include "PhysicalDevice.hpp"
#include "Swapchain.hpp"

//--------------------------------------------------------------------------------------------------
VulkanBackend::Swapchain::Swapchain(
  std::unique_ptr<PhysicalDevice>& physicalDevice,
  std::unique_ptr<Device>& device,
  VkSurfaceKHR& surface,
  uint32_t width,
  uint32_t height)
{
  vkb::SwapchainBuilder swapchainBuilder{physicalDevice->getHandle().physical_device, device->getHandle().device, surface};

  _swapchainImageFormat = VK_FORMAT_B8G8R8A8_UNORM;

  _vkbHandle = swapchainBuilder
                 //.use_default_format_selection()
                 .set_desired_format(
                   VkSurfaceFormatKHR{.format = _swapchainImageFormat, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
                 //use vsync present mode
                 .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
                 .set_desired_extent(width, height)
                 .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
                 .build()
                 .value();

  _swapchainExtent = _vkbHandle.extent;
  _swapchainImages = _vkbHandle.get_images().value();
  _swapchainImageViews = _vkbHandle.get_image_views().value();
}
