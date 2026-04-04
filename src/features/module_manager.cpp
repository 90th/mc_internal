#include "mc_internal/features/module_manager.hpp"

#include <string>

#include "imgui.h"

#include "mc_internal/context.hpp"
#include "mc_internal/features/debug_hud_module.hpp"
#include "mc_internal/features/esp_module.hpp"

namespace mc_internal {

void ModuleManager::Initialize() const {
  if (initialized_) { return; }

  auto debug_hud = std::make_unique<DebugHudModule>();
  debug_hud->toggle();
  modules_.push_back(std::move(debug_hud));

  auto esp = std::make_unique<EspModule>();
  esp->toggle();
  modules_.push_back(std::move(esp));

  initialized_ = true;
}

void ModuleManager::Render3d(const OverlayContext& ctx) const {
  if (!initialized_) { Initialize(); }

  for (const auto& module : modules_) {
    if (module->is_enabled()) { module->on_render_3d(ctx); }
  }
}

void ModuleManager::RenderUi(const OverlayContext& ctx) const {
  if (!initialized_) { Initialize(); }

  if (ctx.show_menu) {
    ImGui::SetNextWindowPos(ImVec2(40.0f, 220.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(260.0f, 0.0f), ImGuiCond_FirstUseEver);

    ImGui::Begin("modules");
    for (const auto& module : modules_) {
      bool enabled = module->is_enabled();
      const std::string label(module->get_name());
      if (ImGui::Checkbox(label.c_str(), &enabled)) { module->toggle(); }
    }
    ImGui::End();
  }

  for (const auto& module : modules_) {
    if (module->is_enabled()) { module->on_render_ui(ctx); }
  }
}

}  // namespace mc_internal
