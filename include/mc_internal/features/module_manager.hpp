#pragma once

#include <memory>
#include <vector>

#include "mc_internal/features/module.hpp"

namespace mc_internal {

struct OverlayContext;

class ModuleManager {
 public:
  void Initialize() const;
  void Render3d(const OverlayContext& ctx) const;
  void RenderUi(const OverlayContext& ctx) const;

 private:
  mutable bool initialized_ = false;
  mutable std::vector<std::unique_ptr<Module>> modules_;
};

}  // namespace mc_internal
