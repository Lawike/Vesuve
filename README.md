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
"environment": {
  ...
  "VULKAN_SDK": "",
  ...
}
```
And provide the path to the libraries.
For the vulkan SDK provide the root directory, not the **Bin** that contains the executables.

### Know issues
When trying to debug shaders i encountered an issue :
the executable wasn't able to find a specific DLL, i had to provide the directory in my PATH variable :
`C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.40.33807\bin\Hostx64\x64`

