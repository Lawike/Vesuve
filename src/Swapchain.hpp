#pragma once
#include "Swapchain.hpp"
#include "VkBootstrap.h"
#include "VkTypes.hpp"

namespace VulkanBackend
{
  class Swapchain
  {
   public:
    Swapchain(
      std::unique_ptr<PhysicalDevice>& physicalDevice,
      std::unique_ptr<Device>& device,
      VkSurfaceKHR& surface,
      uint32_t width,
      uint32_t height);

    vkb::Swapchain getHandle() const
    {
      return _vkbHandle;
    }

    VkFormat getSwapchainImageFormat() const
    {
      return _swapchainImageFormat;
    }

    std::vector<VkImage> getSwapchainImages() const
    {
      return _swapchainImages;
    }

    std::vector<VkImageView> getSwapchainImageViews() const
    {
      return _swapchainImageViews;
    }

    VkExtent2D getSwapchainExtent() const
    {
      return _swapchainExtent;
    }

    // Needed as public to gather pointer ref
    vkb::Swapchain _vkbHandle;
    VkFormat _swapchainImageFormat{};
   private:
    std::vector<VkImage> _swapchainImages;
    std::vector<VkImageView> _swapchainImageViews;
    VkExtent2D _swapchainExtent{};
  };

}  // namespace VulkanBackend
