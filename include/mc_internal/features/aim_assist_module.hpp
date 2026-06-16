#pragma once

#include "mc_internal/features/module.hpp"

namespace mc_internal {

class AimAssistModule : public Module {
 public:
  AimAssistModule();

  void on_render_settings(const OverlayContext& ctx) override;
  void on_render_3d(const OverlayContext& ctx) override;

 private:
  float fov_degrees_ = 25.0f;
  float smoothness_ = 8.0f;
  float max_distance_ = 96.0f;
  bool line_of_sight_only_ = true;
  bool target_players_ = true;
  bool target_hostiles_ = false;
};

}  // namespace mc_internal
