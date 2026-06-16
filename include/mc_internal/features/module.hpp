#pragma once

namespace mc_internal {

struct OverlayContext;

enum class ModuleCategory {
  kCombat,
  kVisuals,
  kMovement,
  kMisc,
};

class Module {
 public:
  Module(const char* name, const char* description, ModuleCategory category)
      : category_(category), name_(name), description_(description) {}

  virtual ~Module() = default;

  virtual void on_enable() {}
  virtual void on_disable() {}

  // this stays reserved for floating overlays like the debug hud.
  virtual void on_render_ui(const OverlayContext& ctx) { static_cast<void>(ctx); }

  // compact controls rendered beside the module title when enabled.
  [[nodiscard]] virtual bool has_header_controls() const noexcept { return false; }
  virtual void on_render_header_controls(const OverlayContext& ctx) { static_cast<void>(ctx); }

  // this stays reserved for widgets rendered inside the master menu.
  virtual void on_render_settings(const OverlayContext& ctx) { static_cast<void>(ctx); }

  virtual void on_render_3d(const OverlayContext& ctx) { static_cast<void>(ctx); }

  void toggle() {
    enabled_ = !enabled_;
    if (enabled_) {
      on_enable();
    } else {
      on_disable();
    }
  }

  [[nodiscard]] bool is_enabled() const noexcept { return enabled_; }
  [[nodiscard]] ModuleCategory get_category() const noexcept { return category_; }
  [[nodiscard]] const char* get_name() const noexcept { return name_; }
  [[nodiscard]] const char* get_description() const noexcept { return description_; }

 protected:
  bool enabled_ = false;
  ModuleCategory category_ = ModuleCategory::kMisc;
  const char* name_ = nullptr;
  const char* description_ = nullptr;
};

}  // namespace mc_internal
