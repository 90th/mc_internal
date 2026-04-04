#pragma once

#include <array>

#include "mc_internal/features/module.hpp"

namespace mc_internal {

class EspModule : public Module {
 public:
  EspModule();

  void on_render_settings(const OverlayContext& ctx) override;
  void on_render_3d(const OverlayContext& ctx) override;

 private:
  float max_render_distance_ = 256.0f;
  bool show_players_ = true;
  std::array<float, 4> esp_color_ = {0.86f, 0.15f, 0.15f, 1.0f};
};

}  // namespace mc_internal
