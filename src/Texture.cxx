#include <cmath>
#include <stdexcept>
#include "Buffer.hpp"
#include "SingleTimeCommand.hpp"
#include "Texture.hpp"

//------------------------------------------------------------------------------------------------
Texture::Texture(
  VkPhysicalDevice physicalDevice,
  VkDevice device,
  int width,
  int height,
  int channels,
  stbi_uc* pixels,
  VkCommandPool commandPool,
  VkQueue graphicsQueue)
{
  this->physicalDevice = physicalDevice;
  this->device = device;
  this->width = width;
  this->height = height;
  this->channels = channels;
  this->pixels = pixels;
  this->commandPool = commandPool;
  this->graphicsQueue = graphicsQueue;
  this->mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;

  this->textureImage = VesuveImage{
    physicalDevice,
    device,
    static_cast<uint32_t>(width),
    static_cast<uint32_t>(height),
    mipLevels,
    VK_SAMPLE_COUNT_1_BIT,
    VK_FORMAT_R8G8B8A8_SRGB,
    VK_IMAGE_TILING_OPTIMAL,
    VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT};

  this->createTextureImage();
}

//------------------------------------------------------------------------------------------------
void Texture::createTextureImage()
{
  VkDeviceSize imageSize = width * height * 4;

  Buffer stagingBuffer{
    physicalDevice,
    device,
    imageSize,
    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};

  void* data;
  vkMapMemory(device, stagingBuffer.memory, 0, imageSize, 0, &data);
  memcpy(data, pixels, static_cast<size_t>(imageSize));
  vkUnmapMemory(device, stagingBuffer.memory);
  stbi_image_free(pixels);

  textureImage.transitionImageLayout(
    VK_FORMAT_R8G8B8A8_SRGB,
    VK_IMAGE_LAYOUT_UNDEFINED,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    mipLevels,
    commandPool,
    graphicsQueue);


  this->copyBufferToImage(
    stagingBuffer.refBuffer, textureImage.image, static_cast<uint32_t>(width), static_cast<uint32_t>(height));

  this->generateMipmaps(textureImage.image, VK_FORMAT_R8G8B8A8_SRGB);

  vkDestroyBuffer(device, stagingBuffer.refBuffer, nullptr);
  vkFreeMemory(device, stagingBuffer.memory, nullptr);
}

//------------------------------------------------------------------------------------------------
void Texture::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
  SingleTimeCommand command{device, commandPool, graphicsQueue};
  command.begin();
  VkBufferImageCopy region{};
  region.bufferOffset = 0;
  region.bufferRowLength = 0;
  region.bufferImageHeight = 0;

  region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.mipLevel = 0;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount = 1;

  region.imageOffset = {0, 0, 0};
  region.imageExtent = {width, height, 1};

  vkCmdCopyBufferToImage(command.buffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
  command.end();
}

//------------------------------------------------------------------------------------------------
void Texture::generateMipmaps(VkImage image, VkFormat imageFormat)
{
  // Check if image format supports linear blitting
  VkFormatProperties formatProperties;
  vkGetPhysicalDeviceFormatProperties(physicalDevice, imageFormat, &formatProperties);
  if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
  {
    throw std::runtime_error("texture image format does not support linear blitting!");
  }

  SingleTimeCommand command{device, commandPool, graphicsQueue};
  command.begin();

  VkImageMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.image = image;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;
  barrier.subresourceRange.levelCount = 1;

  int32_t mipWidth = width;
  int32_t mipHeight = height;

  for (uint32_t i = 1; i < mipLevels; i++)
  {
    barrier.subresourceRange.baseMipLevel = i - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

    vkCmdPipelineBarrier(
      command.buffer,
      VK_PIPELINE_STAGE_TRANSFER_BIT,
      VK_PIPELINE_STAGE_TRANSFER_BIT,
      0,
      0,
      nullptr,
      0,
      nullptr,
      1,
      &barrier);

    VkImageBlit blit{};
    blit.srcOffsets[0] = {0, 0, 0};
    blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
    blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.srcSubresource.mipLevel = i - 1;
    blit.srcSubresource.baseArrayLayer = 0;
    blit.srcSubresource.layerCount = 1;
    blit.dstOffsets[0] = {0, 0, 0};
    blit.dstOffsets[1] = {mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1};
    blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.dstSubresource.mipLevel = i;
    blit.dstSubresource.baseArrayLayer = 0;
    blit.dstSubresource.layerCount = 1;

    vkCmdBlitImage(
      command.buffer,
      image,
      VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
      image,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      1,
      &blit,
      VK_FILTER_LINEAR);

    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(
      command.buffer,
      VK_PIPELINE_STAGE_TRANSFER_BIT,
      VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
      0,
      0,
      nullptr,
      0,
      nullptr,
      1,
      &barrier);
    if (mipWidth > 1)
      mipWidth /= 2;
    if (mipHeight > 1)
      mipHeight /= 2;
  }

  barrier.subresourceRange.baseMipLevel = mipLevels - 1;
  barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

  vkCmdPipelineBarrier(
    command.buffer,
    VK_PIPELINE_STAGE_TRANSFER_BIT,
    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
    0,
    0,
    nullptr,
    0,
    nullptr,
    1,
    &barrier);
  command.end();
}
