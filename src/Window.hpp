#pragma once

//#include <functional>
//#include <vector>
#include "VkTypes.hpp"
#ifndef SDL_H
#define SDL_H
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#endif

class Window
{
 public:
  Window(VkExtent2D windowExtent);
  ~Window();

  // Window instance properties.
  SDL_Window* handle() const
  {
    return _sdlHandle;
  }
  bool isMinimized() const;
  std::vector<const char*> GetRequiredInstanceExtensions() const;

  int pollEvent(SDL_Event& e);
  /**
  * Process the window events
  * @returns true if the event is SDL_Quit, false otherwise.
  */
  bool processEvent(SDL_Event& e);
 private:
  SDL_Window* _sdlHandle = nullptr;
  bool _isMinimized = false;
  std::vector<const char*> _deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};  // Supported device extensions
};
