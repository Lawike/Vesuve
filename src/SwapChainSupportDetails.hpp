#pragma once

#include <vector>
#ifndef SDL_H
#define SDL_H
#include "SDL2/SDL.h"
#include "SDL2/SDL_vulkan.h"
#endif
#include "VkTypes.hpp"

class SwapChainSupportDetails
{
 public:
  SwapChainSupportDetails() = default;
  SwapChainSupportDetails(VkSurfaceKHR& surface);
  ~SwapChainSupportDetails() = default;

  VkSurfaceCapabilitiesKHR capabilities = VkSurfaceCapabilitiesKHR{};
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> presentModes;
  VkSurfaceKHR surface = VkSurfaceKHR{};

  void querySwapChainSupport(VkPhysicalDevice& device);
  VkSurfaceFormatKHR chooseSwapSurfaceFormat();
  VkPresentModeKHR chooseSwapPresentMode();
  VkExtent2D chooseSwapExtent(SDL_Window* window);
};
