#include "mc_internal/ui/widgets.hpp"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_internal.h"

namespace mc_internal::ui {

constexpr ImU32 kAccentColor = IM_COL32(180, 20, 20, 255);
constexpr ImU32 kDimTextColor = IM_COL32(127, 127, 127, 255);
constexpr ImU32 kWhiteTextColor = IM_COL32(255, 255, 255, 255);
constexpr ImU32 kTrackColor = IM_COL32(60, 60, 60, 255);

bool Tab(const char* label, bool selected) {
  ImGuiWindow* window = ImGui::GetCurrentWindow();
  if (window->SkipItems) return false;

  ImGuiContext& g = *GImGui;
  const ImGuiID id = window->GetID(label);
  const ImVec2 label_size = ImGui::CalcTextSize(label, nullptr, true);

  // Add padding around the text for the clickable area
  const ImVec2 pos = window->DC.CursorPos;
  const ImVec2 size = ImVec2(label_size.x + 16.0f, label_size.y + 12.0f);
  const ImRect bb(pos, pos + size);

  ImGui::ItemSize(size, 0.0f);
  if (!ImGui::ItemAdd(bb, id)) return false;

  bool hovered, held;
  bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);

  // Smooth color interpolation for text
  float t = hovered ? 1.0f : 0.0f;
  if (selected) t = 1.0f;

  ImU32 text_color =
      selected ? kAccentColor
               : ImGui::GetColorU32(ImLerp(ImGui::ColorConvertU32ToFloat4(kDimTextColor),
                                           ImGui::ColorConvertU32ToFloat4(kWhiteTextColor),
                                           t));

  // Center the text inside the bounding box
  ImVec2 text_pos = ImVec2(pos.x + 8.0f, pos.y + 6.0f);
  window->DrawList->AddText(text_pos, text_color, label);

  // Draw the crisp 2px active line underneath if selected
  if (selected) {
    window->DrawList->AddLine(ImVec2(pos.x + 4.0f, bb.Max.y - 2.0f),
                              ImVec2(bb.Max.x - 4.0f, bb.Max.y - 2.0f),
                              kAccentColor,
                              2.0f);
  }

  return pressed;
}

bool SliderFloat(const char* label, float* v, float v_min, float v_max, const char* format) {
  ImGuiWindow* window = ImGui::GetCurrentWindow();
  if (window->SkipItems) return false;

  ImGuiContext& g = *GImGui;
  const ImGuiID id = window->GetID(label);
  const ImVec2 label_size = ImGui::CalcTextSize(label, nullptr, true);

  // Calculate responsive width
  const float w = ImGui::CalcItemWidth();
  const ImVec2 pos = window->DC.CursorPos;
  const ImRect bb(pos, pos + ImVec2(w, label_size.y + g.Style.FramePadding.y * 2.0f));

  ImGui::ItemSize(bb, g.Style.FramePadding.y);
  if (!ImGui::ItemAdd(bb, id)) return false;

  bool hovered, held;
  bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);

  // Slider Logic
  bool value_changed = false;
  if (held) {
    const float mouse_x = g.IO.MousePos.x;
    const float normalized_val = ImClamp((mouse_x - bb.Min.x) / w, 0.0f, 1.0f);
    const float new_value = v_min + (v_max - v_min) * normalized_val;
    if (*v != new_value) {
      *v = new_value;
      value_changed = true;
    }
  }

  // Draw the minimalist line track
  const float track_y = bb.Min.y + (bb.GetHeight() * 0.5f);
  const float normalized_current = ImClamp((*v - v_min) / (v_max - v_min), 0.0f, 1.0f);
  const float grab_x = bb.Min.x + (w * normalized_current);

  // Background track (gray)
  window->DrawList->AddLine(
      ImVec2(bb.Min.x, track_y), ImVec2(bb.Max.x, track_y), kTrackColor, 3.0f);

  // Active track (red fill)
  window->DrawList->AddLine(ImVec2(bb.Min.x, track_y), ImVec2(grab_x, track_y), kAccentColor, 3.0f);

  // Grabber handle (white circle)
  window->DrawList->AddCircleFilled(ImVec2(grab_x, track_y), 5.0f, kWhiteTextColor);

  // Render value text to the right of the slider
  char value_buf[64];
  snprintf(value_buf, sizeof(value_buf), format, *v);
  window->DrawList->AddText(
      ImVec2(bb.Max.x + g.Style.ItemInnerSpacing.x, bb.Min.y + g.Style.FramePadding.y),
      kDimTextColor,
      value_buf);

  return value_changed;
}

}  // namespace mc_internal::ui