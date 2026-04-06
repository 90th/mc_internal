#include "mc_internal/ui/widgets.hpp"

#include <cstdio>

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_internal.h"

#include "mc_internal/ui/anim.hpp"

namespace mc_internal::ui {

// ── palette ──────────────────────────────────────────────────────────────────
constexpr ImU32 kAccent = IM_COL32(200, 60, 60, 255);
constexpr ImU32 kAccentDim = IM_COL32(200, 60, 60, 100);
constexpr ImU32 kTextBright = IM_COL32(220, 220, 220, 255);
constexpr ImU32 kTextMid = IM_COL32(140, 140, 140, 255);
constexpr ImU32 kTextDim = IM_COL32(90, 90, 90, 255);
constexpr ImU32 kTrack = IM_COL32(38, 38, 42, 255);
constexpr ImU32 kCardBg = IM_COL32(30, 30, 34, 255);
constexpr ImU32 kToggleOff = IM_COL32(50, 50, 55, 255);

// ── Tab ──────────────────────────────────────────────────────────────────────
bool Tab(const char* label, bool selected) {
  ImGuiWindow* window = ImGui::GetCurrentWindow();
  if (window->SkipItems) { return false; }

  const ImGuiID id = window->GetID(label);
  const ImVec2 label_size = ImGui::CalcTextSize(label, nullptr, true);

  const ImVec2 pos = window->DC.CursorPos;
  const ImVec2 size = ImVec2(label_size.x + 14.0f, label_size.y + 8.0f);
  const ImRect bb(pos, pos + size);

  ImGui::ItemSize(size, 0.0f);
  if (!ImGui::ItemAdd(bb, id)) { return false; }

  bool hovered, held;
  const bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);

  const float ct = Anim::Get(id + 0x10000, selected ? 1.0f : 0.0f, 8.0f);
  const ImU32 text_color = LerpColor(hovered ? kTextBright : kTextMid, kAccent, ct);

  const ImVec2 text_pos = ImVec2(pos.x + 7.0f, pos.y + 4.0f);
  window->DrawList->AddText(text_pos, text_color, label);

  const float ul = Anim::Get(id + 0x20000, selected ? 1.0f : 0.0f, 8.0f);
  if (ul > 0.01f) {
    const ImU32 ul_color = IM_COL32(200, 60, 60, static_cast<int>(255.0f * ul));
    window->DrawList->AddRectFilled(
        ImVec2(pos.x + 4.0f, bb.Max.y - 2.0f), ImVec2(bb.Max.x - 4.0f, bb.Max.y), ul_color, 1.0f);
  }

  return pressed;
}

// ── SliderFloat ──────────────────────────────────────────────────────────────
bool SliderFloat(const char* label, float* v, float v_min, float v_max, const char* format) {
  ImGuiWindow* window = ImGui::GetCurrentWindow();
  if (window->SkipItems) { return false; }

  ImGuiContext& g = *GImGui;
  const ImGuiStyle& style = g.Style;
  const ImGuiID id = window->GetID(label);
  const float width = ImGui::CalcItemWidth();

  constexpr float kTrackHeight = 6.0f;
  constexpr float kGrabRadius = 5.0f;
  const float total_height = kTrackHeight + kGrabRadius * 2.0f + 4.0f;
  const ImVec2 pos = window->DC.CursorPos;
  const ImRect bb(pos, pos + ImVec2(width, total_height));

  ImGui::ItemSize(bb, style.FramePadding.y);
  if (!ImGui::ItemAdd(bb, id)) { return false; }

  bool hovered, held;
  ImGui::ButtonBehavior(bb, id, &hovered, &held);

  bool value_changed = false;
  const float value_range = v_max - v_min;
  if (held && value_range > 0.0f) {
    const float normalized_val = ImClamp((g.IO.MousePos.x - bb.Min.x) / width, 0.0f, 1.0f);
    const float new_value = v_min + value_range * normalized_val;
    if (*v != new_value) {
      *v = new_value;
      value_changed = true;
    }
  }

  if (value_changed) { ImGui::MarkItemEdited(id); }

  const float track_y = bb.Min.y + (total_height - kTrackHeight) * 0.5f;
  const ImRect track_bb(ImVec2(bb.Min.x, track_y), ImVec2(bb.Max.x, track_y + kTrackHeight));
  const float track_rounding = kTrackHeight * 0.5f;

  const float normalized_current =
      value_range > 0.0f ? ImClamp((*v - v_min) / value_range, 0.0f, 1.0f) : 0.0f;
  const float fill_x = track_bb.Min.x + track_bb.GetWidth() * normalized_current;

  window->DrawList->AddRectFilled(track_bb.Min, track_bb.Max, kTrack, track_rounding);
  if (fill_x > track_bb.Min.x + 1.0f) {
    window->DrawList->AddRectFilled(
        track_bb.Min, ImVec2(fill_x, track_bb.Max.y), kAccent, track_rounding);
  }

  const ImVec2 grab_center(fill_x, track_bb.GetCenter().y);
  const ImU32 grab_color = (held || hovered) ? kTextBright : IM_COL32(200, 200, 200, 255);
  window->DrawList->AddCircleFilled(grab_center, kGrabRadius, grab_color);

  char value_buf[64];
  std::snprintf(value_buf, sizeof(value_buf), format, *v);
  const ImVec2 value_size = ImGui::CalcTextSize(value_buf, nullptr, true);
  const ImVec2 text_pos(bb.Max.x - value_size.x, track_bb.Max.y + 2.0f);
  window->DrawList->AddText(text_pos, kTextMid, value_buf);

  return value_changed;
}

// ── Toggle ───────────────────────────────────────────────────────────────────
bool Toggle(const char* label, bool* v) {
  ImGuiWindow* window = ImGui::GetCurrentWindow();
  if (window->SkipItems) { return false; }

  const ImGuiID id = window->GetID(label);
  const ImVec2 label_size = ImGui::CalcTextSize(label, nullptr, true);

  // compact rectangular toggle: 28x14
  constexpr float kToggleW = 28.0f;
  constexpr float kToggleH = 14.0f;
  constexpr float kKnobSize = 10.0f;
  constexpr float kRounding = 3.0f;

  const float total_width = kToggleW + 6.0f + label_size.x;
  const float total_height = ImMax(kToggleH, label_size.y);
  const ImVec2 pos = window->DC.CursorPos;
  const ImRect bb(pos, pos + ImVec2(total_width, total_height));

  ImGui::ItemSize(bb, 0.0f);
  if (!ImGui::ItemAdd(bb, id)) { return false; }

  bool hovered, held;
  const bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);
  if (pressed) { *v = !*v; }

  const float track_y = pos.y + (total_height - kToggleH) * 0.5f;
  const ImVec2 track_min(pos.x, track_y);
  const ImVec2 track_max(pos.x + kToggleW, track_y + kToggleH);

  const float t = Anim::Get(id, *v ? 1.0f : 0.0f, 8.0f);
  const ImU32 track_color = LerpColor(kToggleOff, kAccent, t);
  window->DrawList->AddRectFilled(track_min, track_max, track_color, kRounding);

  const float knob_pad = (kToggleH - kKnobSize) * 0.5f;
  const float knob_off = track_min.x + knob_pad;
  const float knob_on = track_max.x - kKnobSize - knob_pad;
  const float knob_x = knob_off + (knob_on - knob_off) * t;
  const float knob_y = track_y + knob_pad;
  window->DrawList->AddRectFilled(
      ImVec2(knob_x, knob_y), ImVec2(knob_x + kKnobSize, knob_y + kKnobSize), kTextBright, 2.0f);

  const ImVec2 text_pos(pos.x + kToggleW + 6.0f, pos.y + (total_height - label_size.y) * 0.5f);
  window->DrawList->AddText(text_pos, kTextBright, label);

  return pressed;
}

// ── SectionHeader ────────────────────────────────────────────────────────────
void SectionHeader(const char* label) {
  ImGuiWindow* window = ImGui::GetCurrentWindow();
  if (window->SkipItems) { return; }

  ImGui::Spacing();

  const ImVec2 pos = window->DC.CursorPos;
  const ImVec2 label_size = ImGui::CalcTextSize(label, nullptr, true);
  const float avail_width = ImGui::GetContentRegionAvail().x;

  window->DrawList->AddText(pos, kTextDim, label);

  const float line_y = pos.y + label_size.y + 3.0f;
  window->DrawList->AddLine(
      ImVec2(pos.x, line_y), ImVec2(pos.x + avail_width, line_y), IM_COL32(50, 50, 55, 255), 1.0f);

  ImGui::Dummy(ImVec2(0.0f, label_size.y + 8.0f));
}

// ── DescriptionText ──────────────────────────────────────────────────────────
void DescriptionText(const char* text) {
  ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.45f, 0.45f, 0.45f, 1.0f));
  ImGui::TextWrapped("%s", text);
  ImGui::PopStyleColor();
}

}  // namespace mc_internal::ui
