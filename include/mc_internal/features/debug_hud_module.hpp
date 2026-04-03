#pragma once

#include "mc_internal/features/module.hpp"

namespace mc_internal {

class DebugHudModule : public Module {
 public:
  DebugHudModule();

  void on_render_ui(const OverlayContext& ctx) override;
};

}  // namespace mc_internal
