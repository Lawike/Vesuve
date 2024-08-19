#pragma once

#include "VkTypes.hpp"

class VkEngine
{
 public:
  ~VkEngine();
  VkEngine(const VkEngine&) = delete;
  VkEngine(VkEngine&&) = delete;
  VkEngine& operator=(const VkEngine&) = delete;
  VkEngine& operator=(VkEngine&&) = delete;

  bool _isInitialized{false};
  int _frameNumber{0};
  bool stop_rendering{false};
  VkExtent2D _windowExtent{1700, 900};

  struct SDL_Window* _window{nullptr};

  static VkEngine& Get();

  // initializes everything in the engine
  void init();

  // shuts down the engine
  void cleanup();

  // draw loop
  void draw();

  // run main loop
  void run();
};
