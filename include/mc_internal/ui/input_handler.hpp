#pragma once

#include "mc_internal/context.hpp"

struct GLFWwindow;

namespace mc_internal {

// Polls keyboard and mouse state from GLFW and feeds events into ImGui.
// Handles the INSERT key toggle and cursor mode transitions.
void ProcessInput(GLFWwindow* window, OverlayContext& ctx);

}  // namespace mc_internal
