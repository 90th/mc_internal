#pragma once

#include "mc_internal/context.hpp"

struct GLFWwindow;

namespace mc_internal {

// Renders the overlay menu window and custom cursor.
// No-op when ctx.show_menu is false.
void RenderMenu(GLFWwindow* window, const OverlayContext& ctx);

}  // namespace mc_internal
