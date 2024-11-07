#pragma once
#include "VkTypes.hpp"

class VkEngine;

struct Stats
{
  float frametime;
  float meshDrawTime;
  float sceneUpdateTime;
  int triangleCount;
  int drawcallCount;
};

static class UserInterface
{
 public:
  static void init(VkEngine* engine);
  static void display(VkEngine* engine);
  static void displayBackground(VkEngine* engine);
  static void displaySceneSelector(VkEngine* engine);
  static void displayLighting(VkEngine* engine);
};
