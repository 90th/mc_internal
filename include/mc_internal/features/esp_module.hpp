#pragma once

#include "mc_internal/features/module.hpp"

namespace mc_internal {

class EspModule : public Module {
 public:
  EspModule();

  void on_render_3d(const OverlayContext& ctx) override;
};

}  // namespace mc_internal
