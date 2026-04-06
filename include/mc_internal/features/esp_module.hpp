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
  struct TargetGroupState {
    bool enabled = false;
    std::array<float, 4> color = {1.0f, 1.0f, 1.0f, 1.0f};
  };

  float max_render_distance_ = 256.0f;
  TargetGroupState player_group_ = {true, {0.86f, 0.15f, 0.15f, 1.0f}};
  TargetGroupState hostile_group_ = {true, {0.98f, 0.36f, 0.22f, 1.0f}};
  TargetGroupState passive_group_ = {false, {0.24f, 0.78f, 0.42f, 1.0f}};
  TargetGroupState item_group_ = {false, {0.92f, 0.76f, 0.20f, 1.0f}};
};

}  // namespace mc_internal
