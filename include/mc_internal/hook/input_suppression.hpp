#pragma once

struct GLFWwindow;

namespace mc_internal {

struct OverlayContext;

// Installs inline hooks on GLFW input functions so that gameplay input
// (keyboard, mouse, cursor) is suppressed while the overlay menu is open.
// Callback setter hooks intercept game-registered callbacks and wrap them
// in filtering proxies; polling hooks return neutral values.
//
// After installation the overlay's own polling pointers in ctx.glfw are
// redirected to the hook trampolines so the overlay always reads real
// hardware state regardless of suppression.
//
// Returns true if at least one hook was installed successfully.
bool InstallInputSuppression(OverlayContext& ctx);

// Captures callbacks that the game already registered with GLFW (before our
// hooks went live) and replaces them with filtering proxies. Must be called
// exactly once from the render thread after the target window is known.
void CaptureGameCallbacks(GLFWwindow* window);

}  // namespace mc_internal
