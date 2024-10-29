#include "Window.hpp"

//--------------------------------------------------------------------------------------------------
Window::Window(VkExtent2D windowExtent)
{
  // We initialize SDL and create a window with it.
  SDL_SetMainReady();
  SDL_Init(SDL_INIT_VIDEO);
  SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
  _sdlHandle = SDL_CreateWindow(
    "Vesuve Vulkan Engine",
    SDL_WINDOWPOS_UNDEFINED,
    SDL_WINDOWPOS_UNDEFINED,
    windowExtent.width,
    windowExtent.height,
    window_flags);
}

//--------------------------------------------------------------------------------------------------
Window::~Window()
{
  if (_sdlHandle != nullptr)
  {
    SDL_DestroyWindow(_sdlHandle);
  }
}

//--------------------------------------------------------------------------------------------------
bool Window::isMinimized() const
{
  return _isMinimized;
}

//--------------------------------------------------------------------------------------------------
std::vector<const char*> Window::GetRequiredInstanceExtensions() const
{
  uint32_t sdlExtensionCount = 0;
  std::vector<const char*> sdlExtensions;
  SDL_Vulkan_GetInstanceExtensions(_sdlHandle, &sdlExtensionCount, sdlExtensions.data());
  return sdlExtensions;
}

//--------------------------------------------------------------------------------------------------
int Window::pollEvent(SDL_Event& e)
{
  return SDL_PollEvent(&e);
}

//--------------------------------------------------------------------------------------------------
bool Window::processEvent(SDL_Event& e)
{
  //close the window when user alt-f4s or clicks the X button
  if (e.type == SDL_QUIT)
    return true;

  if (e.type == SDL_WINDOWEVENT)
  {
    if (e.window.event == SDL_WINDOWEVENT_MINIMIZED)
    {
      _isMinimized = true;
    }
    if (e.window.event == SDL_WINDOWEVENT_RESTORED)
    {
      _isMinimized = false;
    }
  }
  return false;
}
