#include "Image.hpp"
#include "VkInitializers.hpp"

VulkanBackend::Image::Image(VkExtent2D _windowExtent, VkFormat format, VkImageUsageFlags usage, VmaAllocator allocator)
{
  //draw image size will match the window
  VkExtent3D drawImageExtent = {_windowExtent.width, _windowExtent.height, 1};

  //hardcoding the draw format to 32 bit float
  _handle.imageFormat = format;
  _handle.imageExtent = drawImageExtent;

  VkImageCreateInfo imgInfo = vkinit::imageCreateInfo(_handle.imageFormat, usage, drawImageExtent);

  //for the draw image, we want to allocate it from gpu local memory
  VmaAllocationCreateInfo imgAllocInfo = {};
  imgAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
  imgAllocInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  //allocate and create the image
  vmaCreateImage(allocator, &imgInfo, &imgAllocInfo, &_handle.image, &_handle.allocation, nullptr);
}
