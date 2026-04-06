#pragma once

#include "mc_internal/context.hpp"

struct GLFWwindow;

namespace mc_internal {

// renders the overlay menu.
// no-op when ctx.show_menu is false.
void RenderMenu(GLFWwindow* window, const OverlayContext& ctx);

}  // namespace mc_internal
