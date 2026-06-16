#pragma once

#include <string>

#include "mc_internal/features/module.hpp"

namespace mc_internal {

class AimAssistModule : public Module {
 public:
  AimAssistModule();

  void on_render_settings(const OverlayContext& ctx) override;
  [[nodiscard]] bool has_header_controls() const noexcept override { return true; }
  void on_render_header_controls(const OverlayContext& ctx) override;
  void on_render_3d(const OverlayContext& ctx) override;

 private:
  enum class ActivationBindType {
    kKeyboard,
    kMouse,
  };

  [[nodiscard]] bool is_activation_held(const OverlayContext& ctx) const;
  [[nodiscard]] std::string activation_bind_label() const;
  void update_bind_capture(const OverlayContext& ctx);

  float fov_degrees_ = 25.0f;
  float smoothness_ = 8.0f;
  float max_distance_ = 96.0f;
  int locked_target_id_ = 0;
  int activation_bind_code_ = 0;
  ActivationBindType activation_bind_type_ = ActivationBindType::kMouse;
  bool activation_was_down_ = false;
  bool waiting_for_bind_ = false;
  bool bind_capture_armed_ = false;
  bool line_of_sight_only_ = true;
  bool target_players_ = true;
  bool target_hostiles_ = false;
};

}  // namespace mc_internal
