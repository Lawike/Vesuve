# Vesuve
This project is an homemade 3D Renderer using Vulkan.
It is higly WIP and motsly a way for me to learn computer graphics and the API.

### Dependencies
At first, this project requires some external dependencies :
- [Vulkan](https://vulkan.lunarg.com/)
- [SDL2](https://github.com/libsdl-org/SDL/)
- [Imgui](https://github.com/ocornut/imgui)
- [stb](https://github.com/nothings/stb)

Please get and install those before continuing.

### Build Guide
This project uses CMake as a build system, there is a `CMakePresets.json` provided as a template to quickly get started.
You'll have to edit the following `cacheVariables` :
```json
"cacheVariables": {
  ...
  "VULKAN_SDK": "",
  "STB_DIR": "",
  ...
}
```
And provide the path to the libraries.
For the vulkan SDK provide the root directory, not the **Bin** that contains the executables.
