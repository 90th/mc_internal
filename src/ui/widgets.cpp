#include "mc_internal/ui/widgets.hpp"

#include <cstdio>

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_internal.h"

namespace mc_internal::ui {

constexpr ImU32 kAccentColor = IM_COL32(180, 20, 20, 255);
constexpr ImU32 kDimTextColor = IM_COL32(127, 127, 127, 255);
constexpr ImU32 kWhiteTextColor = IM_COL32(255, 255, 255, 255);
constexpr ImU32 kTrackColor = IM_COL32(60, 60, 60, 255);
constexpr ImU32 kGrabShadowColor = IM_COL32(0, 0, 0, 70);

bool Tab(const char* label, bool selected) {
  ImGuiWindow* window = ImGui::GetCurrentWindow();
  if (window->SkipItems) { return false; }

  const ImGuiID id = window->GetID(label);
  const ImVec2 label_size = ImGui::CalcTextSize(label, nullptr, true);

  // keep the tab hit box slightly larger than the text.
  const ImVec2 pos = window->DC.CursorPos;
  const ImVec2 size = ImVec2(label_size.x + 16.0f, label_size.y + 12.0f);
  const ImRect bb(pos, pos + size);

  ImGui::ItemSize(size, 0.0f);
  if (!ImGui::ItemAdd(bb, id)) { return false; }

  bool hovered, held;
  const bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);

  float t = hovered ? 1.0f : 0.0f;
  if (selected) { t = 1.0f; }

  const ImU32 text_color =
      selected ? kAccentColor
               : ImGui::GetColorU32(ImLerp(ImGui::ColorConvertU32ToFloat4(kDimTextColor),
                                           ImGui::ColorConvertU32ToFloat4(kWhiteTextColor),
                                           t));

  const ImVec2 text_pos = ImVec2(pos.x + 8.0f, pos.y + 6.0f);
  window->DrawList->AddText(text_pos, text_color, label);

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
  if (window->SkipItems) { return false; }

  ImGuiContext& g = *GImGui;
  const ImGuiStyle& style = g.Style;
  const ImGuiID id = window->GetID(label);
  const float width = ImGui::CalcItemWidth();
  const float frame_height = ImGui::GetFrameHeight();
  const ImVec2 pos = window->DC.CursorPos;
  const ImRect bb(pos, pos + ImVec2(width, frame_height));

  ImGui::ItemSize(bb, style.FramePadding.y);
  if (!ImGui::ItemAdd(bb, id)) { return false; }

  bool hovered, held;
  ImGui::ButtonBehavior(bb, id, &hovered, &held);

  bool value_changed = false;
  const float value_range = v_max - v_min;
  if (held && value_range > 0.0f) {
    const float mouse_x = g.IO.MousePos.x;
    const float normalized_val = ImClamp((mouse_x - bb.Min.x) / width, 0.0f, 1.0f);
    const float new_value = v_min + value_range * normalized_val;
    if (*v != new_value) {
      *v = new_value;
      value_changed = true;
    }
  }

  if (value_changed) { ImGui::MarkItemEdited(id); }

  const ImRect track_bb(bb.Min + ImVec2(0.0f, 2.0f), bb.Max - ImVec2(0.0f, 2.0f));
  const float track_rounding = track_bb.GetHeight() * 0.5f;
  const float normalized_current =
      value_range > 0.0f ? ImClamp((*v - v_min) / value_range, 0.0f, 1.0f) : 0.0f;
  const float fill_x = track_bb.Min.x + track_bb.GetWidth() * normalized_current;
  const ImVec2 grab_center(fill_x, track_bb.GetCenter().y);

  // keep the track tall enough to hold the value text inside it.
  window->DrawList->AddRectFilled(track_bb.Min, track_bb.Max, kTrackColor, track_rounding);
  if (fill_x > track_bb.Min.x) {
    window->DrawList->AddRectFilled(
        track_bb.Min, ImVec2(fill_x, track_bb.Max.y), kAccentColor, track_rounding);
  }
  window->DrawList->AddCircleFilled(grab_center + ImVec2(0.0f, 1.5f), 8.5f, kGrabShadowColor);
  window->DrawList->AddCircleFilled(grab_center, 8.0f, kWhiteTextColor);

  char value_buf[64];
  std::snprintf(value_buf, sizeof(value_buf), format, *v);

  const ImVec2 value_size = ImGui::CalcTextSize(value_buf, nullptr, true);
  const float text_padding_x = 10.0f;
  const ImVec2 text_pos(track_bb.Max.x - text_padding_x - value_size.x,
                        track_bb.Min.y + (track_bb.GetHeight() - value_size.y) * 0.5f);
  window->DrawList->AddText(text_pos + ImVec2(1.0f, 1.0f), IM_COL32(0, 0, 0, 170), value_buf);
  window->DrawList->AddText(text_pos, kWhiteTextColor, value_buf);

  return value_changed;
}

}  // namespace mc_internal::ui
