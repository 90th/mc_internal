#include "mc_internal/ui/widgets.hpp"

#include <cctype>
#include <cstdio>
#include <cstring>

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

// ── HelpMarker ───────────────────────────────────────────────────────────────
void HelpMarker(const char* text) {
  ImGui::TextDisabled("(?)");
  if (!ImGui::IsItemHovered()) { return; }

  ImGui::BeginTooltip();
  ImGui::PushTextWrapPos(ImGui::GetFontSize() * 26.0f);
  ImGui::TextUnformatted(text);
  ImGui::PopTextWrapPos();
  ImGui::EndTooltip();
}

// ── SectionHeader ────────────────────────────────────────────────────────────
void SectionHeader(const char* label, const char* tooltip) {
  ImGuiWindow* window = ImGui::GetCurrentWindow();
  if (window->SkipItems) { return; }

  ImGui::Spacing();

  const ImVec2 pos = window->DC.CursorPos;
  ImGui::TextDisabled("%s", label);
  if (tooltip != nullptr && tooltip[0] != '\0') {
    ImGui::SameLine(0.0f, 4.0f);
    HelpMarker(tooltip);
  }

  const float line_y = ImGui::GetCursorScreenPos().y + 3.0f;
  window->DrawList->AddLine(ImVec2(pos.x, line_y),
                            ImVec2(pos.x + ImGui::GetContentRegionAvail().x, line_y),
                            IM_COL32(50, 50, 55, 255),
                            1.0f);

  ImGui::Dummy(ImVec2(0.0f, 7.0f));
}

// ── LabeledSlider ────────────────────────────────────────────────────────
bool LabeledSlider(const char* label,
                   const char* slider_id,
                   float* v,
                   float v_min,
                   float v_max,
                   const char* format) {
  ImGui::AlignTextToFramePadding();
  ImGui::TextDisabled("%s", label);
  ImGui::SameLine(0.0f, 8.0f);
  ImGui::PushItemWidth(-1.0f);
  bool changed = SliderFloat(slider_id, v, v_min, v_max, format);
  ImGui::PopItemWidth();
  return changed;
}

// ── DescriptionText ──────────────────────────────────────────────────────────
void DescriptionText(const char* text) {
  ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.45f, 0.45f, 0.45f, 1.0f));
  ImGui::TextWrapped("%s", text);
  ImGui::PopStyleColor();
}

// ── FilteredChecklist ────────────────────────────────────────────────────────

namespace {

bool CaseInsensitiveContains(const char* haystack, const char* needle) {
  if (!needle[0]) return true;
  for (const char* h = haystack; *h; ++h) {
    const char* hi = h;
    const char* ni = needle;
    while (*hi && *ni &&
           std::tolower(static_cast<unsigned char>(*hi)) ==
               std::tolower(static_cast<unsigned char>(*ni))) {
      ++hi;
      ++ni;
    }
    if (!*ni) return true;
  }
  return false;
}

}  // namespace

bool FilteredChecklist(const char* id,
                       FilteredChecklistState& state,
                       const char* const* labels,
                       bool* values,
                       int count,
                       float preview_width) {
  bool changed = false;

  int hidden_count = 0;
  for (int i = 0; i < count; ++i) {
    if (!values[i]) ++hidden_count;
  }

  const int visible_count = count - hidden_count;
  char preview[32];
  if (hidden_count == 0) {
    std::snprintf(preview, sizeof(preview), "all");
  } else if (visible_count == 0) {
    std::snprintf(preview, sizeof(preview), "none");
  } else {
    std::snprintf(preview, sizeof(preview), "%d/%d", visible_count, count);
  }

  ImGui::PushItemWidth(preview_width);
  const char* popup_id = id;

  char button_id[128];
  std::snprintf(button_id, sizeof(button_id), "%s###%s_btn", preview, id);

  if (ImGui::Button(button_id, ImVec2(preview_width, 0))) {
    ImGui::OpenPopup(popup_id);
    state.popup_open = true;
    state.search_buf[0] = '\0';
  }
  ImGui::PopItemWidth();

  constexpr float kPopupWidth = 220.0f;
  constexpr float kListHeight = 200.0f;

  ImGui::SetNextWindowSize(ImVec2(kPopupWidth, 0.0f));
  if (ImGui::BeginPopup(popup_id)) {
    ImGui::PushItemWidth(-1.0f);
    if (state.popup_open) {
      ImGui::SetKeyboardFocusHere();
      state.popup_open = false;
    }
    ImGui::InputTextWithHint("##search", "search...", state.search_buf, sizeof(state.search_buf));
    ImGui::PopItemWidth();

    ImGui::Spacing();

    if (ImGui::BeginChild("##list", ImVec2(0.0f, kListHeight), ImGuiChildFlags_None)) {
      int matches = 0;
      for (int i = 0; i < count; ++i) {
        if (state.search_buf[0] && !CaseInsensitiveContains(labels[i], state.search_buf)) {
          continue;
        }
        ++matches;
        if (ImGui::Checkbox(labels[i], &values[i])) { changed = true; }
      }
      if (matches == 0) { ImGui::TextDisabled("no matches"); }
    }
    ImGui::EndChild();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    const float avail = ImGui::GetContentRegionAvail().x;
    const float spacing = ImGui::GetStyle().ItemSpacing.x;
    const float btn_w = (avail - spacing * 2.0f) / 3.0f;

    if (ImGui::Button("show", ImVec2(btn_w, 0))) {
      for (int i = 0; i < count; ++i) values[i] = true;
      changed = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("hide", ImVec2(btn_w, 0))) {
      for (int i = 0; i < count; ++i) values[i] = false;
      changed = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("invert", ImVec2(btn_w, 0))) {
      for (int i = 0; i < count; ++i) values[i] = !values[i];
      changed = true;
    }

    ImGui::EndPopup();
  }

  return changed;
}

// ── TargetGroupRow ──────────────────────────────────────────────────────────
void TargetGroupRow(const char* label,
                    const char* style_id,
                    const char* color_id,
                    bool* enabled,
                    float* color,
                    int* style,
                    float combo_x,
                    float color_x,
                    int color_flags) {
  ImGui::Checkbox(label, enabled);
  if (*enabled) {
    ImGui::SameLine();
    ImGui::SetCursorPosX(combo_x);
    ImGui::PushItemWidth(72.0f);
    ImGui::Combo(style_id, style, "corner\0box\0");
    ImGui::PopItemWidth();
  }
  ImGui::SameLine();
  ImGui::SetCursorPosX(color_x);
  ImGui::ColorEdit4(color_id, color, color_flags);
}

}  // namespace mc_internal::ui
