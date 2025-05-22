# Vesuve
This project is an homemade 3D Renderer using Vulkan.
It is highly WIP and mostly a way for me to learn computer graphics and the API.

## Features
- [x] Rasterizer Pipeline
- [x] Compute Pipeline (for background)
- [x] GLTF Loader
- [x] FPS like camera
- [x] Point Light
- [x] Blinn Phong lighting
- [ ] Ray tracing pipeline (WIP)
  - [ ] Acceleration structure
    - [ ] BLAS
    - [ ] TLAS
  - [x] Raygen shader
  - [x] Closest hit shader
  - [x] Miss Shader
## Dependencies
Most of the dependencies are retrieved using CMake FetchContent. They should be downloaded when configuring the CMake project.
The only prerequisite are [CMake](https://cmake.org/download/) and the [Vulkan SDK](https://vulkan.lunarg.com/) that must be installed on your environment.

## Build Guide
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

You'll need at first to configure the project using CMake, this step will also compile the shaders.
Then build it using CMake, I personnally use the CMake integration from Visual Studio.

If you don't know how to use CMake please check this [ressource](https://cmake.org/cmake/help/latest/guide/tutorial/A%20Basic%20Starting%20Point.html)

## Known issues
When trying to debug shaders I encountered an issue :
the executable wasn't able to find a specific DLL, I had to provide the directory in my `PATH` variable :
`C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.40.33807\bin\Hostx64\x64`

