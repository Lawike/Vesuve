#include "Image.hpp"
#include "VkInitializers.hpp"

VulkanBackend::Image::Image(
  std::unique_ptr<Device>& device,
  VkExtent3D imageExtent,
  VkFormat format,
  VkImageUsageFlags usage,
  VmaAllocator allocator,
  bool mipmapped)
{
  //hardcoding the draw format to 32 bit float
  _handle.imageFormat = format;
  _handle.imageExtent = imageExtent;

  VkImageCreateInfo imgInfo = vkinit::imageCreateInfo(_handle.imageFormat, usage, imageExtent);
  if (mipmapped)
  {
    imgInfo.mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(imageExtent.width, imageExtent.height)))) + 1;
  }

  // always allocate images on dedicated GPU memory  VmaAllocationCreateInfo imgAllocInfo = {};
  VmaAllocationCreateInfo imgAllocInfo = {};
  imgAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
  imgAllocInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  //allocate and create the image
  vmaCreateImage(allocator, &imgInfo, &imgAllocInfo, &_handle.image, &_handle.allocation, nullptr);

  // if the format is a depth format, we will need to have it use the correct
  // aspect flag
  VkImageAspectFlags aspectFlag = format == VK_FORMAT_D32_SFLOAT ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;

  //build a image-view for the draw image to use for rendering
  VkImageViewCreateInfo viewInfo = vkinit::imageViewCreateInfo(_handle.imageFormat, _handle.image, aspectFlag);
  viewInfo.subresourceRange.levelCount = imgInfo.mipLevels;

  VK_CHECK(vkCreateImageView(device->getHandle(), &viewInfo, nullptr, &_handle.imageView));
}
