#pragma once

struct GLFWwindow;

namespace mc_internal {

struct OverlayContext;

// Installs inline hooks on GLFW callback-setter functions so that gameplay
// input (keyboard, mouse, cursor) is suppressed while the overlay menu is
// open.  Each hooked setter intercepts game-registered callbacks and wraps
// them in filtering proxies that drop events when the menu is visible.
//
// Returns true if at least one hook was installed successfully.
bool InstallInputSuppression(OverlayContext& ctx);

// Captures callbacks that the game already registered with GLFW (before our
// hooks went live) and replaces them with filtering proxies. Must be called
// exactly once from the render thread after the target window is known.
void CaptureGameCallbacks(GLFWwindow* window);

}  // namespace mc_internal
