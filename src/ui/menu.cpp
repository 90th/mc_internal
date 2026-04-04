#include "mc_internal/ui/menu.hpp"

#include <algorithm>
#include <array>

#include "imgui.h"

#include "mc_internal/features/module.hpp"

namespace mc_internal {

namespace {

constexpr ImVec4 kSelectedCategoryColor = ImVec4(0.647f, 0.231f, 0.231f, 1.0f);

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
  if (ctx.show_menu) {
    static ModuleCategory selected_category = ModuleCategory::kVisuals;

    ImGui::SetNextWindowPos(ImVec2(40.0f, 40.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(
        ImVec2(std::max(760.0f, static_cast<float>(ctx.window_width) * 0.58f),
               std::max(460.0f, static_cast<float>(ctx.window_height) * 0.62f)),
        ImGuiCond_FirstUseEver);

    ImGui::Begin("mc internal", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse);

    ImGui::BeginChild("dashboard_header", ImVec2(0.0f, 48.0f), true);
    ImGui::AlignTextToFramePadding();
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 6.0f);
    ImGui::TextUnformatted("mc internal");

    constexpr std::array<ModuleCategory, 4> kCategories = {ModuleCategory::kCombat,
                                                           ModuleCategory::kVisuals,
                                                           ModuleCategory::kMovement,
                                                           ModuleCategory::kMisc};

    float category_offset = 170.0f;
    for (ModuleCategory category : kCategories) {
      ImGui::SameLine(category_offset);

      // cache the selected state so push and pop are perfectly balanced!
      const bool is_selected = (selected_category == category);

      if (is_selected) { ImGui::PushStyleColor(ImGuiCol_Text, kSelectedCategoryColor); }

      if (ImGui::Selectable(ToDisplayName(category), is_selected, 0, ImVec2(96.0f, 28.0f))) {
        selected_category = category;
      }

      if (is_selected) { ImGui::PopStyleColor(); }

      category_offset += 104.0f;
    }
    ImGui::EndChild();

    ImGui::Spacing();

    ImGui::BeginChild(
        "dashboard_content", ImVec2(0.0f, 0.0f), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);
    ImGui::TextDisabled("category");
    ImGui::SameLine();
    ImGui::TextUnformatted(ToDisplayName(selected_category));
    ImGui::Spacing();

    bool rendered_any_module = false;
    for (const auto& module : ctx.module_manager.get_modules()) {
      if (module->get_category() != selected_category) { continue; }

      rendered_any_module = true;
      const ImVec2 available = ImGui::GetContentRegionAvail();

      // safely isolate this module's widgets to avoid checkbox ID collisions
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
  }

  ctx.module_manager.RenderUi(ctx);

  if (!ctx.show_menu) { return; }

  // draw a custom dark red cursor on top of everything
  ImDrawList* fg = ImGui::GetForegroundDrawList();
  const ImVec2 m = ImGui::GetIO().MousePos;
  const ImU32 cursor_color = IM_COL32(180, 20, 20, 255);
  const ImU32 border_color = IM_COL32(0, 0, 0, 255);

  // border first for visibility
  fg->AddTriangle(
      m, ImVec2(m.x + 13.0f, m.y + 13.0f), ImVec2(m.x, m.y + 18.0f), border_color, 1.5f);

  // inner red fill
  fg->AddTriangleFilled(
      m, ImVec2(m.x + 13.0f, m.y + 13.0f), ImVec2(m.x, m.y + 18.0f), cursor_color);
}

}  // namespace mc_internal