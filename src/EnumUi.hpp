// ========================================
// Fichier: EnumUI.h
// Generic system for enum comboBox
// ========================================

#pragma once
#include <imgui.h>
#include <array>
#include <string_view>
#include <type_traits>

namespace UI
{

  // base template for enum metaData
  template<typename EnumType> struct EnumTraits;

  // utilitary function to obtain the enum name
  template<typename EnumType> constexpr std::string_view GetEnumName(EnumType value)
  {
    const auto& traits = EnumTraits<EnumType>{};
    for (size_t i = 0; i < traits.values.size(); ++i)
    {
      if (traits.values[i] == value)
      {
        return traits.names[i];
      }
    }
    return "Unknown";
  }

  // Utilitary function to obtain the enum index/value
  template<typename EnumType> constexpr int GetEnumIndex(EnumType value)
  {
    const auto& traits = EnumTraits<EnumType>{};
    for (size_t i = 0; i < traits.values.size(); ++i)
    {
      if (traits.values[i] == value)
      {
        return static_cast<int>(i);
      }
    }
    return 0;
  }

  // Generic combobox for enum
  template<typename EnumType> bool ComboBox(const char* label, EnumType& current_value)
  {
    static_assert(std::is_enum_v<EnumType>, "EnumType must be an enum");

    const auto& traits = EnumTraits<EnumType>{};
    int current_index = GetEnumIndex(current_value);

    // imGui requires a const char* array
    std::array<const char*, traits.names.size()> items;
    for (size_t i = 0; i < traits.names.size(); ++i)
    {
      items[i] = traits.names[i].data();
    }

    bool changed = ImGui::Combo(label, &current_index, items.data(), static_cast<int>(items.size()));

    if (changed)
    {
      current_value = traits.values[current_index];
    }

    return changed;
  }

}  // namespace UI
