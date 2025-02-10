# Vesuve
This project is an homemade 3D Renderer using Vulkan.
It is higly WIP and motsly a way for me to learn computer graphics and the API.

### Dependencies
Most of the dependencies are retrieved and built using CMake FetchContent. They should be downloaded and built when configuring the CMake project.
The only prerequisite is the [Vulkan SDK](https://vulkan.lunarg.com/) that must be installed on your environment.

### Build Guide
This project uses CMake as a build system, there is a `CMakePresets.json` provided as a template to quickly get started.
You'll have to fill the following empty `cacheVariables` :
```json
"cacheVariables": {
  ...
  "VULKAN_SDK": "",
  ...
}
```
And provide the path to the libraries.
For the vulkan SDK provide the root directory, not the **Bin** that contains the executables.