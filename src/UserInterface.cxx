#ifndef IMGUI_H
#define IMGUI_H
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_vulkan.h>
#endif
#include "UserInterface.hpp"
#include "VkEngine.hpp"

//--------------------------------------------------------------------------------------------------
void UserInterface::init(VkEngine* engine)
{
  // 1: create descriptor pool for IMGUI
  //  the size of the pool is very oversize, but it's copied from imgui demo
  //  itself.
  VkDescriptorPoolSize pool_sizes[] = {
    {VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
    {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
    {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
    {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
    {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
    {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
    {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
    {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
    {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
    {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
    {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}};

  VkDescriptorPoolCreateInfo poolInfo = {};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
  poolInfo.maxSets = 1000;
  poolInfo.poolSizeCount = (uint32_t)std::size(pool_sizes);
  poolInfo.pPoolSizes = pool_sizes;

  VkDescriptorPool imguiPool;
  VK_CHECK(vkCreateDescriptorPool(engine->_device->getHandle(), &poolInfo, nullptr, &imguiPool));

  // 2: initialize imgui library

  // this initializes the core structures of imgui
  ImGui::CreateContext();

  // this initializes imgui for SDL
  ImGui_ImplSDL2_InitForVulkan(engine->_window->handle());

  // this initializes imgui for Vulkan
  ImGui_ImplVulkan_InitInfo initInfo = {};
  initInfo.Instance = engine->_instance->getHandle();
  initInfo.PhysicalDevice = engine->_chosenGPU->getHandle();
  initInfo.Device = engine->_device->getHandle();
  initInfo.Queue = engine->_device->getGraphicsQueue();
  initInfo.DescriptorPool = imguiPool;
  initInfo.MinImageCount = 3;
  initInfo.ImageCount = 3;
  initInfo.UseDynamicRendering = true;

  //dynamic rendering parameters for imgui to use
  initInfo.PipelineRenderingCreateInfo = {.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
  initInfo.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
  initInfo.PipelineRenderingCreateInfo.pColorAttachmentFormats = &engine->_swapchain->_swapchainImageFormat;

  initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

  ImGui_ImplVulkan_Init(&initInfo);

  ImGui_ImplVulkan_CreateFontsTexture();

  // add the destroy the imgui created structures
  engine->_deletionQueue.push(
    [=]()
    {
      ImGui_ImplVulkan_Shutdown();
      vkDestroyDescriptorPool(engine->_device->getHandle(), imguiPool, nullptr);
    });
}

//--------------------------------------------------------------------------------------------------
void UserInterface::display(VkEngine* engine)
{
  // imgui new frame
  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplSDL2_NewFrame();

  ImGui::NewFrame();

  ImGui::Begin("Stats");
  ImGui::Text("frametime %f ms", engine->_stats.frametime);
  ImGui::Text("draw time %f ms", engine->_stats.meshDrawTime);
  ImGui::Text("update time %f ms", engine->_stats.sceneUpdateTime);
  ImGui::Text("triangles %i", engine->_stats.triangleCount);
  ImGui::Text("draws %i", engine->_stats.drawcallCount);
  ImGui::Text("frame index %i", engine->_frameNumber);
  ImGui::End();

  displayBackground(engine);
  displaySceneSelector(engine);
  displayLighting(engine);
  displayRenderingModeSelector(engine);

  ImGui::Render();
}

//--------------------------------------------------------------------------------------------------
void UserInterface::displayBackground(VkEngine* engine)
{
  if (ImGui::Begin("background"))
  {
    ImGui::SliderFloat("Render Scale", &engine->renderScale, 0.3f, 1.f);
    ComputeEffect& selected = *engine->_backgroundEffects[engine->_currentBackgroundEffect];

    ImGui::Text("Selected effect: ", selected.name);

    ImGui::SliderInt("Effect Index", &engine->_currentBackgroundEffect, 0, engine->_backgroundEffects.size() - 1);

    ImGui::InputFloat4("data1", (float*)&selected.data.data1);
    ImGui::InputFloat4("data2", (float*)&selected.data.data2);
    ImGui::InputFloat4("data3", (float*)&selected.data.data3);
    ImGui::InputFloat4("data4", (float*)&selected.data.data4);
  }
  ImGui::End();
}

//--------------------------------------------------------------------------------------------------
void UserInterface::displaySceneSelector(VkEngine* engine)
{
  // Extract keys from unordered_map into a vector of strings
  std::vector<std::string> keys;
  for (const auto& pair : engine->_loadedNodes)
  {
    keys.push_back(pair.first.c_str());
  }
  // Display the combo box
  if (ImGui::BeginCombo("Loaded nodes", engine->_selectedNodeName.c_str()))
  {
    for (size_t i = 0; i < keys.size(); i++)
    {
      bool isSelected = keys[i] == engine->_selectedNodeName;
      if (ImGui::Selectable(keys[i].c_str(), isSelected))
      {
        engine->_selectedNodeName = keys[i];
      }
    }
    ImGui::EndCombo();
  }
}

//--------------------------------------------------------------------------------------------------
void UserInterface::displayLighting(VkEngine* engine)
{
  if (ImGui::Begin("Lighting"))
  {
    ImGui::InputFloat4("Light position", (float*)&engine->_mainLight.position);
    ImGui::InputFloat4("Light color", (float*)&engine->_mainLight.color);
    ImGui::InputFloat("Light power", (float*)&engine->_mainLight.power);

    ImGui::InputFloat4("Camera position", (float*)&engine->_mainCamera.position);
    ImGui::InputFloat("Camera yaw", (float*)&engine->_mainCamera.yaw);

    ImGui::InputFloat("Ambient coefficient", (float*)&engine->_mainSurfaceProperties.ambientCoefficient);
    ImGui::InputFloat("Specular coefficient", (float*)&engine->_mainSurfaceProperties.specularCoefficient);
    ImGui::InputFloat("Shininess", (float*)&engine->_mainSurfaceProperties.shininess);
    ImGui::InputFloat("Gamma", (float*)&engine->_mainSurfaceProperties.screenGamma);
  }
  ImGui::End();
}

//--------------------------------------------------------------------------------------------------
void UserInterface::displayRenderingModeSelector(VkEngine* engine)
{
  if (ImGui::Begin("Rendering Mode Selector"))
  {
    ImGui::Checkbox("Raytracing", (bool*)&engine->_isRaytracingEnabled);
  }
  ImGui::End();
}
