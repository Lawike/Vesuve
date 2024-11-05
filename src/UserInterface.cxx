#ifndef IMGUI_H
#define IMGUI_H
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_vulkan.h>
#endif
#include "UserInterface.hpp"
#include "VkEngine.hpp"

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
  ImGui::End();

  displayBackground(engine);
  displaySceneSelector(engine);
  displayLighting(engine);

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

    ImGui::End();
  }
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

    ImGui::End();
  }
}
