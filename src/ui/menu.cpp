#include "mc_internal/ui/menu.hpp"

#include <algorithm>
#include <array>

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_internal.h"

#include "mc_internal/features/module.hpp"
#include "mc_internal/ui/anim.hpp"
#include "mc_internal/ui/widgets.hpp"

namespace mc_internal {

namespace {

// ── layout constants ─────────────────────────────────────────────────────────
constexpr float kSidebarWidth = 130.0f;
constexpr float kHeaderHeight = 36.0f;
constexpr float kCardRounding = 4.0f;

// ── colors ───────────────────────────────────────────────────────────────────
constexpr ImU32 kAccent = IM_COL32(200, 60, 60, 255);
constexpr ImU32 kAccentSoft = IM_COL32(200, 60, 60, 40);
constexpr ImU32 kTitleColor = IM_COL32(200, 60, 60, 255);
constexpr ImU32 kTextBright = IM_COL32(220, 220, 220, 255);
constexpr ImU32 kTextMid = IM_COL32(140, 140, 140, 255);
constexpr ImU32 kTextDim = IM_COL32(90, 90, 90, 255);
constexpr ImU32 kCardBg = IM_COL32(28, 29, 34, 255);
constexpr ImU32 kCardBorder = IM_COL32(45, 46, 52, 255);
constexpr ImU32 kSidebarBg = IM_COL32(22, 23, 27, 255);
constexpr ImU32 kSidebarHover = IM_COL32(35, 36, 42, 255);
constexpr ImU32 kSidebarActive = IM_COL32(200, 60, 60, 20);
constexpr ImU32 kSeparator = IM_COL32(45, 46, 52, 255);

const char* ToDisplayName(ModuleCategory category) {
  switch (category) {
    case ModuleCategory::kCombat:
      return "combat";
    case ModuleCategory::kVisuals:
      return "visuals";
    case ModuleCategory::kMovement:
      return "movement";
    case ModuleCategory::kMisc:
      return "misc";
  }
  return "misc";
}

void RenderModuleCard(const std::unique_ptr<Module>& module, const OverlayContext& ctx) {
  ImGuiWindow* window = ImGui::GetCurrentWindow();
  const float avail_w = ImGui::GetContentRegionAvail().x;
  ImGui::PushID(module.get());

  ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, kCardRounding);
  ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::ColorConvertU32ToFloat4(kCardBg));
  ImGui::PushStyleColor(ImGuiCol_Border, ImGui::ColorConvertU32ToFloat4(kCardBorder));

  if (ImGui::BeginChild("mod_card",
                        ImVec2(avail_w, 0.0f),
                        ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY,
                        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
    // ── card header ──
    bool enabled = module->is_enabled();

    if (ImGui::Checkbox(module->get_name(), &enabled)) { module->toggle(); }

    if (module->get_description() != nullptr) {
      ImGui::SameLine();
      ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(kTextDim));
      const ImVec2 desc_size = ImGui::CalcTextSize(module->get_description(), nullptr, true);
      const float space = ImGui::GetContentRegionAvail().x - desc_size.x;
      if (space > 0.0f) { ImGui::SetCursorPosX(ImGui::GetCursorPosX() + space); }
      ImGui::TextUnformatted(module->get_description());
      ImGui::PopStyleColor();
    }

    // ── card body ──
    if (module->is_enabled()) {
      ImGui::Spacing();
      ImDrawList* dl = ImGui::GetWindowDrawList();
      const ImVec2 sep_start = ImGui::GetCursorScreenPos();
      const float sep_w = ImGui::GetContentRegionAvail().x;
      dl->AddLine(sep_start, ImVec2(sep_start.x + sep_w, sep_start.y), kSeparator, 1.0f);
      ImGui::Dummy(ImVec2(0.0f, 4.0f));

      module->on_render_settings(ctx);
    }
  }
  ImGui::EndChild();

  ImGui::PopStyleColor(2);
  ImGui::PopStyleVar();
  ImGui::PopID();

  ImGui::Spacing();
}

}  // namespace

void RenderMenu(GLFWwindow* window, const OverlayContext& ctx) {
  static_cast<void>(window);

  static float menu_alpha = 0.0f;

  menu_alpha += (ctx.show_menu ? 1.0f : -1.0f) * ImGui::GetIO().DeltaTime * 10.0f;
  menu_alpha = std::clamp(menu_alpha, 0.0f, 1.0f);

  ctx.module_manager.RenderUi(ctx);

  if (menu_alpha <= 0.001f) { return; }

  static ModuleCategory selected_category = ModuleCategory::kVisuals;

  // ── window setup ───────────────────────────────────────────────────────
  const float menu_w = std::clamp(static_cast<float>(ctx.window_width) * 0.46f, 540.0f, 780.0f);
  const float menu_h = std::clamp(static_cast<float>(ctx.window_height) * 0.52f, 340.0f, 520.0f);
  ImGui::SetNextWindowPos(ImVec2(40.0f, 40.0f), ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowSize(ImVec2(menu_w, menu_h), ImGuiCond_FirstUseEver);

  ImGui::PushStyleVar(ImGuiStyleVar_Alpha, menu_alpha);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

  ImGui::Begin("##mc_menu",
               nullptr,
               ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                   ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

  const ImVec2 win_pos = ImGui::GetWindowPos();
  const ImVec2 win_size = ImGui::GetWindowSize();
  ImDrawList* dl = ImGui::GetWindowDrawList();

  // ── sidebar ────────────────────────────────────────────────────────────
  {
    const ImVec2 sidebar_min = win_pos;
    const ImVec2 sidebar_max(win_pos.x + kSidebarWidth, win_pos.y + win_size.y);

    dl->AddRectFilled(sidebar_min, sidebar_max, kSidebarBg, 6.0f, ImDrawFlags_RoundCornersLeft);

    const ImVec2 title_pos(sidebar_min.x + 14.0f, sidebar_min.y + 14.0f);
    dl->AddText(title_pos, kTitleColor, "mc internal");

    const float sep_y = title_pos.y + ImGui::GetTextLineHeight() + 8.0f;
    dl->AddLine(ImVec2(sidebar_min.x + 10.0f, sep_y),
                ImVec2(sidebar_max.x - 10.0f, sep_y),
                kSeparator,
                1.0f);

    constexpr std::array<ModuleCategory, 4> kCategories = {ModuleCategory::kCombat,
                                                           ModuleCategory::kVisuals,
                                                           ModuleCategory::kMovement,
                                                           ModuleCategory::kMisc};

    float btn_y = sep_y + 10.0f;
    const float btn_y_start = btn_y;
    constexpr float kBtnHeight = 26.0f;
    constexpr float kBtnPad = 2.0f;

    // animated indicator Y offset (relative to first button Y)
    static float indicator_offset = 0.0f;
    static bool indicator_initialized = false;
    float target_offset = 0.0f;

    {
      float off = 0.0f;
      for (ModuleCategory category : kCategories) {
        if (category == selected_category) { target_offset = off; }
        off += kBtnHeight + kBtnPad;
      }
    }

    if (!indicator_initialized) {
      indicator_offset = target_offset;
      indicator_initialized = true;
    } else {
      const float dt = ImGui::GetIO().DeltaTime;
      indicator_offset += (target_offset - indicator_offset) * std::min(12.0f * dt, 1.0f);
    }

    for (ModuleCategory category : kCategories) {
      const char* name = ToDisplayName(category);
      const bool is_selected = (category == selected_category);
      const ImVec2 btn_min(sidebar_min.x + 6.0f, btn_y);
      const ImVec2 btn_max(sidebar_max.x - 6.0f, btn_y + kBtnHeight);

      ImGui::SetCursorScreenPos(btn_min);
      ImGui::PushID(static_cast<int>(category));
      if (ImGui::InvisibleButton("##cat", ImVec2(btn_max.x - btn_min.x, kBtnHeight))) {
        selected_category = category;
      }
      const bool hovered = ImGui::IsItemHovered();
      ImGui::PopID();

      if (is_selected) {
        dl->AddRectFilled(btn_min, btn_max, kSidebarActive, 3.0f);
      } else if (hovered) {
        dl->AddRectFilled(btn_min, btn_max, kSidebarHover, 3.0f);
      }

      // animated text color per button
      const ImGuiID color_id = ImGui::GetID(name);
      const float sel_t = ui::Anim::Get(color_id, is_selected ? 1.0f : 0.0f, 8.0f);
      ImU32 text_col;
      if (sel_t > 0.01f) {
        text_col = ui::LerpColor(hovered ? kTextBright : kTextMid, kAccent, sel_t);
      } else {
        text_col = hovered ? kTextBright : kTextMid;
      }

      const ImVec2 text_size = ImGui::CalcTextSize(name, nullptr, true);
      const ImVec2 text_pos(btn_min.x + 14.0f, btn_min.y + (kBtnHeight - text_size.y) * 0.5f);
      dl->AddText(text_pos, text_col, name);

      btn_y += kBtnHeight + kBtnPad;
    }

    const float ind_y = btn_y_start + indicator_offset;
    dl->AddRectFilled(ImVec2(sidebar_min.x + 6.0f, ind_y + 3.0f),
                      ImVec2(sidebar_min.x + 6.0f + 3.0f, ind_y + kBtnHeight - 3.0f),
                      kAccent,
                      1.5f);
  }

  // ── vertical separator between sidebar and content ─────────────────────
  dl->AddLine(ImVec2(win_pos.x + kSidebarWidth, win_pos.y + 8.0f),
              ImVec2(win_pos.x + kSidebarWidth, win_pos.y + win_size.y - 8.0f),
              kSeparator,
              1.0f);

  // ── content panel ──────────────────────────────────────────────────────
  {
    const float content_x = kSidebarWidth + 1.0f;
    const float content_w = win_size.x - content_x;

    ImGui::SetCursorScreenPos(ImVec2(win_pos.x + content_x, win_pos.y));

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 10.0f));
    ImGui::BeginChild("##content",
                      ImVec2(content_w, win_size.y),
                      ImGuiChildFlags_AlwaysUseWindowPadding,
                      ImGuiWindowFlags_None);

    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(kTextDim));
    ImGui::TextUnformatted(ToDisplayName(selected_category));
    ImGui::PopStyleColor();
    ImGui::Spacing();

    ImGui::BeginChild("##modules", ImVec2(0.0f, 0.0f), false, ImGuiWindowFlags_None);

    bool rendered_any = false;
    for (const auto& module : ctx.module_manager.get_modules()) {
      if (module->get_category() != selected_category) continue;
      rendered_any = true;
      RenderModuleCard(module, ctx);
    }

    if (!rendered_any) {
      ImGui::Spacing();
      ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(kTextDim));
      ImGui::TextUnformatted("no modules in this category");
      ImGui::PopStyleColor();
    }

    ImGui::EndChild();
    ImGui::EndChild();
    ImGui::PopStyleVar();
  }

  ImGui::End();
  ImGui::PopStyleVar(2);

  // ── custom cursor ──────────────────────────────────────────────────────
  ImDrawList* fg = ImGui::GetForegroundDrawList();
  const ImVec2 mp = ImGui::GetIO().MousePos;
  const int alpha = static_cast<int>(255.0f * menu_alpha);
  const ImU32 cursor_fill = IM_COL32(200, 60, 60, alpha);
  const ImU32 cursor_edge = IM_COL32(0, 0, 0, alpha);

  fg->AddTriangle(
      mp, ImVec2(mp.x + 12.0f, mp.y + 12.0f), ImVec2(mp.x, mp.y + 16.0f), cursor_edge, 1.5f);
  fg->AddTriangleFilled(
      mp, ImVec2(mp.x + 12.0f, mp.y + 12.0f), ImVec2(mp.x, mp.y + 16.0f), cursor_fill);
}

}  // namespace mc_internal
