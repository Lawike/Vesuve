# Vesuve
This project is an homemade 3D Renderer using Vulkan.
It is higly WIP and motsly a way for me to learn computer graphics and the API.

## Quick start
### Dependencies
At first, this project requires some external dependencies :
- GLFW
- GLM
- stb_image

Please get those before continuing.

### Build Guide
This project uses CMake as a build system, there is a `CMakePresets.json` provided as a template to quickly get started.
You'll have to edit the following `cacheVariables` :
```json
"cacheVariables": {
  ...
  "GLM_LIB_PATH": "",
  "GLFW_LIB_PATH": "",
  "STB_IMAGE_LIB_PATH": ""
  ...
}
```
And provide the path to the libraries.
