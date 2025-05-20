#pragma once

#include "VkTypes.hpp"

/*
 * Abstractions for Vulkan structures initilization.
 */
namespace vkinit
{
  VkCommandPoolCreateInfo commandPoolCreateInfo(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags = 0);
  VkCommandBufferAllocateInfo commandBufferAllocateInfo(VkCommandPool pool, uint32_t count = 1);

  VkCommandBufferBeginInfo commandBufferBeginInfo(VkCommandBufferUsageFlags flags = 0);
  VkCommandBufferSubmitInfo commandBufferSubmitInfo(VkCommandBuffer& cmd);

  VkFenceCreateInfo fenceCreateInfo(VkFenceCreateFlags flags = 0);

  VkSemaphoreCreateInfo semaphoreCreateInfo(VkSemaphoreCreateFlags flags = 0);

  VkSubmitInfo2 submitInfo(
    VkCommandBufferSubmitInfo* cmd,
    VkSemaphoreSubmitInfo* signalSemaphoreInfo,
    VkSemaphoreSubmitInfo* waitSemaphoreInfo);
  VkPresentInfoKHR presentInfo();

  VkRenderingAttachmentInfo attachmentInfo(VkImageView view, VkClearValue* clear, VkImageLayout layout);

  VkRenderingAttachmentInfo depthAttachmentInfo(VkImageView view, VkImageLayout layout);

  VkRenderingInfo renderingInfo(
    VkExtent2D renderExtent,
    VkRenderingAttachmentInfo* colorAttachment,
    VkRenderingAttachmentInfo* depthAttachment);

  VkImageSubresourceRange imageSubresourceRange(VkImageAspectFlags aspectMask);

  VkSemaphoreSubmitInfo semaphoreSubmitInfo(VkPipelineStageFlags2 stageMask, VkSemaphore semaphore);
  VkDescriptorSetLayoutBinding
  descriptorSetLayoutBinding(VkDescriptorType type, VkShaderStageFlags stageFlags, uint32_t binding);
  VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo(
    VkDescriptorSetLayoutBinding* bindings,
    uint32_t bindingCount);
  VkWriteDescriptorSet
  writeDescriptorImage(VkDescriptorType type, VkDescriptorSet dstSet, VkDescriptorImageInfo* imageInfo, uint32_t binding);
  VkWriteDescriptorSet
  writeDescriptorBuffer(VkDescriptorType type, VkDescriptorSet dstSet, VkDescriptorBufferInfo* bufferInfo, uint32_t binding);
  VkDescriptorBufferInfo bufferInfo(VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range);

  VkImageCreateInfo imageCreateInfo(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent);
  VkImageViewCreateInfo imageViewCreateInfo(VkFormat format, VkImage image, VkImageAspectFlags aspectFlags);
  VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo();
  VkPipelineShaderStageCreateInfo
  pipelineShaderStageCreateInfo(VkShaderStageFlagBits stage, VkShaderModule shaderModule, const char* entry = "main");
}  // namespace vkinit
