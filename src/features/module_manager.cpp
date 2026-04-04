#include "mc_internal/features/module_manager.hpp"

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

  for (const auto& module : modules_) {
    if (module->is_enabled()) { module->on_render_ui(ctx); }
  }
}

}  // namespace mc_internal
