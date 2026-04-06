#include "mc_internal/ui/menu.hpp"

#include <algorithm>
#include <array>

#include "imgui.h"

#include "mc_internal/features/module.hpp"
#include "mc_internal/ui/widgets.hpp"

namespace mc_internal {

namespace {

constexpr float kHeaderHeight = 32.0f;
constexpr float kTabWidthPadding = 16.0f;
constexpr ImU32 kHeaderTitleColor = IM_COL32(180, 180, 180, 255);

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

}  // namespace

void RenderMenu(GLFWwindow* window, const OverlayContext& ctx) {
  static_cast<void>(window);

  static float menu_alpha = 0.0f;

  // keep the menu transition soft without dragging the whole frame.
  menu_alpha += (ctx.show_menu ? 1.0f : -1.0f) * ImGui::GetIO().DeltaTime * 12.0f;
  menu_alpha = std::clamp(menu_alpha, 0.0f, 1.0f);

  // keep world overlays alive even while the menu fades.
  ctx.module_manager.RenderUi(ctx);

  // stop once the fade is effectively gone.
  if (menu_alpha <= 0.001f) { return; }

  static ModuleCategory selected_category = ModuleCategory::kVisuals;

  ImGui::SetNextWindowPos(ImVec2(40.0f, 40.0f), ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowSize(ImVec2(std::max(760.0f, static_cast<float>(ctx.window_width) * 0.58f),
                                  std::max(460.0f, static_cast<float>(ctx.window_height) * 0.62f)),
                           ImGuiCond_FirstUseEver);

  // let the menu fade as one unit.
  ImGui::PushStyleVar(ImGuiStyleVar_Alpha, menu_alpha);

  ImGui::Begin("mc internal", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse);

  constexpr std::array<ModuleCategory, 4> kCategories = {ModuleCategory::kCombat,
                                                         ModuleCategory::kVisuals,
                                                         ModuleCategory::kMovement,
                                                         ModuleCategory::kMisc};

  const ImGuiStyle& style = ImGui::GetStyle();
  float total_tabs_width = 0.0f;
  for (ModuleCategory category : kCategories) {
    total_tabs_width +=
        ImGui::CalcTextSize(ToDisplayName(category), nullptr, true).x + kTabWidthPadding;
  }
  total_tabs_width += style.ItemSpacing.x * static_cast<float>(kCategories.size() - 1);

  const ImVec2 title_size = ImGui::CalcTextSize("mc internal", nullptr, true);
  const ImVec2 tab_label_size = ImGui::CalcTextSize("visuals", nullptr, true);
  const float title_pos_y = (kHeaderHeight - title_size.y) * 0.5f;
  const float tab_pos_y = (kHeaderHeight - (tab_label_size.y + 12.0f)) * 0.5f;

  // keep the header tight so the tabs feel intentional.
  ImGui::BeginChild("dashboard_header",
                    ImVec2(0.0f, kHeaderHeight),
                    false,
                    ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

  const ImVec2 header_origin = ImGui::GetCursorScreenPos();
  ImGui::GetWindowDrawList()->AddText(
      ImVec2(header_origin.x, header_origin.y + title_pos_y), kHeaderTitleColor, "mc internal");

  const float header_width = ImGui::GetWindowWidth();
  const float tab_start_x = header_width * 0.5f - total_tabs_width * 0.5f;
  ImGui::SetCursorPos(ImVec2(tab_start_x, tab_pos_y));

  for (size_t index = 0; index < kCategories.size(); ++index) {
    if (index > 0) { ImGui::SameLine(); }

    const ModuleCategory category = kCategories[index];
    if (ui::Tab(ToDisplayName(category), selected_category == category)) {
      selected_category = category;
    }
  }
  ImGui::EndChild();

  ImGui::Spacing();

  ImGui::BeginChild(
      "dashboard_content", ImVec2(0.0f, 0.0f), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);

  bool rendered_any_module = false;
  for (const auto& module : ctx.module_manager.get_modules()) {
    if (module->get_category() != selected_category) continue;

    rendered_any_module = true;
    const ImVec2 available = ImGui::GetContentRegionAvail();
    ImGui::PushID(module.get());

    if (ImGui::BeginChild("module_card",
                          ImVec2(available.x, 0.0f),
                          ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY,
                          ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
      bool enabled = module->is_enabled();

      if (ImGui::Checkbox(module->get_name(), &enabled)) { module->toggle(); }

      if (module->get_description() != nullptr) {
        ImGui::SameLine();
        ImGui::TextDisabled("%s", module->get_description());
      }

      if (module->is_enabled()) {
        ImGui::Separator();
        module->on_render_settings(ctx);
      }
    }
    ImGui::EndChild();
    ImGui::PopID();
    ImGui::Spacing();
  }

  if (!rendered_any_module) { ImGui::TextDisabled("nothing is registered in this category yet"); }

  ImGui::EndChild();
  ImGui::End();

  ImGui::PopStyleVar();

  // keep the overlay cursor styling even when glfw cursor mode is managed separately.
  ImDrawList* fg = ImGui::GetForegroundDrawList();
  const ImVec2 mouse_pos = ImGui::GetIO().MousePos;
  const ImU32 cursor_color = IM_COL32(180, 20, 20, static_cast<int>(255.0f * menu_alpha));
  const ImU32 border_color = IM_COL32(0, 0, 0, static_cast<int>(255.0f * menu_alpha));

  fg->AddTriangle(mouse_pos,
                  ImVec2(mouse_pos.x + 13.0f, mouse_pos.y + 13.0f),
                  ImVec2(mouse_pos.x, mouse_pos.y + 18.0f),
                  border_color,
                  1.5f);
  fg->AddTriangleFilled(mouse_pos,
                        ImVec2(mouse_pos.x + 13.0f, mouse_pos.y + 13.0f),
                        ImVec2(mouse_pos.x, mouse_pos.y + 18.0f),
                        cursor_color);
}

}  // namespace mc_internal
