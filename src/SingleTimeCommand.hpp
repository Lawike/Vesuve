#pragma once
#include "VkTypes.hpp"

class SingleTimeCommand
{
 public:
  VkCommandBuffer buffer{};
  VkCommandPool commandPool;
  VkDevice device;
  VkQueue graphicsQueue;

  SingleTimeCommand(VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue);

  void begin();
  void end();
};
