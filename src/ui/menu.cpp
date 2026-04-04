#include "mc_internal/ui/menu.hpp"

#include <algorithm>
#include <array>

#include "imgui.h"

#include "mc_internal/features/module.hpp"
#include "mc_internal/ui/widgets.hpp"

namespace mc_internal {

namespace {

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
  static float menu_alpha = 0.0f;

  // Smoothly interpolate alpha based on the menu toggle state
  menu_alpha += (ctx.show_menu ? 1.0f : -1.0f) * ImGui::GetIO().DeltaTime * 12.0f;
  menu_alpha = std::clamp(menu_alpha, 0.0f, 1.0f);

  // Render floating HUDs regardless of menu state
  ctx.module_manager.RenderUi(ctx);

  // If the menu is fully faded out, abort entirely to save cycles and prevent ghost input polling
  if (menu_alpha <= 0.001f) { return; }

  static ModuleCategory selected_category = ModuleCategory::kVisuals;

  ImGui::SetNextWindowPos(ImVec2(40.0f, 40.0f), ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowSize(ImVec2(std::max(760.0f, static_cast<float>(ctx.window_width) * 0.58f),
                                  std::max(460.0f, static_cast<float>(ctx.window_height) * 0.62f)),
                           ImGuiCond_FirstUseEver);

  // Apply the global fade transparency
  ImGui::PushStyleVar(ImGuiStyleVar_Alpha, menu_alpha);

  ImGui::Begin("mc internal", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse);

  // Clean, compact 36px header with no scrollbars
  ImGui::BeginChild("dashboard_header",
                    ImVec2(0.0f, 36.0f),
                    true,
                    ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
  ImGui::AlignTextToFramePadding();
  ImGui::TextUnformatted("mc internal");

  constexpr std::array<ModuleCategory, 4> kCategories = {ModuleCategory::kCombat,
                                                         ModuleCategory::kVisuals,
                                                         ModuleCategory::kMovement,
                                                         ModuleCategory::kMisc};

  float category_offset = 140.0f;
  for (ModuleCategory category : kCategories) {
    ImGui::SameLine(category_offset);

    // Utilize our custom text tab widget
    if (ui::Tab(ToDisplayName(category), selected_category == category)) {
      selected_category = category;
    }
    category_offset += 100.0f;
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

      // Default ImGui checkbox for toggles, as requested
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

  // Restore global alpha
  ImGui::PopStyleVar();

  // Custom Cursor (Only draws if the menu is at least partially visible)
  ImDrawList* fg = ImGui::GetForegroundDrawList();
  const ImVec2 m = ImGui::GetIO().MousePos;

  // Fade the cursor out alongside the menu
  const ImU32 cursor_color = IM_COL32(180, 20, 20, static_cast<int>(255 * menu_alpha));
  const ImU32 border_color = IM_COL32(0, 0, 0, static_cast<int>(255 * menu_alpha));

  fg->AddTriangle(
      m, ImVec2(m.x + 13.0f, m.y + 13.0f), ImVec2(m.x, m.y + 18.0f), border_color, 1.5f);
  fg->AddTriangleFilled(
      m, ImVec2(m.x + 13.0f, m.y + 13.0f), ImVec2(m.x, m.y + 18.0f), cursor_color);
}

}  // namespace mc_internal