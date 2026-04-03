#pragma once

#include <string_view>

namespace mc_internal {

struct OverlayContext;

class Module {
 public:
  Module(std::string_view name, std::string_view description)
      : name_(name), description_(description) {}
  virtual ~Module() = default;

  virtual void on_enable() {}
  virtual void on_disable() {}

  virtual void on_render_ui(const OverlayContext& ctx) { static_cast<void>(ctx); }
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
  [[nodiscard]] std::string_view get_name() const noexcept { return name_; }

 protected:
  bool enabled_ = false;
  std::string_view name_;
  std::string_view description_;
};

}  // namespace mc_internal
