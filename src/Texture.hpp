#pragma once

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif

#ifndef STB_IMAGE_H
#define STB_IMAGE_H
#include "stb_image.h"
#endif  // !STB_IMAGE_H

#include <string>
#include "VesuveImage.hpp"

class Texture
{
 public:
  VesuveImage textureImage;
  int width;
  int height;
  int channels;
  stbi_uc* pixels;
  uint32_t mipLevels;
  VkImageView textureImageView;
  VkSampler textureSampler;
  std::string texturePath;

  Texture() = default;
  Texture(
    VkPhysicalDevice physicalDevice,
    VkDevice device,
    int width,
    int height,
    int channels,
    stbi_uc* pixels,
    VkCommandPool commandPool,
    VkQueue graphicsQueue);

 private:
  VkPhysicalDevice physicalDevice;
  VkDevice device;
  VkCommandPool commandPool;
  VkQueue graphicsQueue;

  void createTextureImage();
  void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
  void generateMipmaps(VkImage image, VkFormat imageFormat);
};
