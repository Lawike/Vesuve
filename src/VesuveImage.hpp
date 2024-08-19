#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif

#include <cstdint>

class VesuveImage
{
 public:
  uint32_t width;
  uint32_t height;
  uint32_t mipLevels;
  VkSampleCountFlagBits numSamples;
  VkFormat format;
  VkImageTiling tiling;
  VkImageUsageFlags usage;
  VkMemoryPropertyFlags properties;
  VkImage image{};
  VkDeviceMemory imageMemory{};

  VesuveImage() = default;
  VesuveImage(
    VkPhysicalDevice physicalDevice,
    VkDevice device,
    uint32_t width,
    uint32_t height,
    uint32_t mipLevels,
    VkSampleCountFlagBits numSamples,
    VkFormat format,
    VkImageTiling tiling,
    VkImageUsageFlags usage,
    VkMemoryPropertyFlags properties);
  void transitionImageLayout(
    VkFormat format,
    VkImageLayout oldLayout,
    VkImageLayout newLayout,
    uint32_t mipLevels,
    VkCommandPool commandPool,
    VkQueue graphicsQueue);


 private:
  VkPhysicalDevice physicalDevice;
  VkDevice device;
  void createImage();
};
