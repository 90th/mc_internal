#include "mc_internal/hook/input_suppression.hpp"

#include <format>
#include <string_view>

#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif
#include <GLFW/glfw3.h>

#include "libmem/libmem.h"

#include "mc_internal/context.hpp"
#include "mc_internal/hook/glfw_bindings.hpp"
#include "mc_internal/utils/logging.hpp"

namespace mc_internal {

namespace {

// ---------------------------------------------------------------------------
// Shared suppression state
// ---------------------------------------------------------------------------

const bool* g_menu_open = nullptr;

// ---------------------------------------------------------------------------
// Trampolines (original functions, bypassing our hooks)
// ---------------------------------------------------------------------------

GlfwSetCursorPosCallbackFn g_orig_set_cursor_pos_cb = nullptr;
GlfwSetKeyCallbackFn g_orig_set_key_cb = nullptr;
GlfwSetMouseButtonCallbackFn g_orig_set_mouse_button_cb = nullptr;
GlfwSetCharCallbackFn g_orig_set_char_cb = nullptr;
GlfwSetScrollCallbackFn g_orig_set_scroll_cb = nullptr;

// ---------------------------------------------------------------------------
// Stored game callbacks (captured at init or when the game re-registers)
// ---------------------------------------------------------------------------

GLFWcursorposfun g_game_cursor_pos = nullptr;
GLFWkeyfun g_game_key = nullptr;
GLFWmousebuttonfun g_game_mouse_button = nullptr;
GLFWcharfun g_game_char = nullptr;
GLFWscrollfun g_game_scroll = nullptr;

// ---------------------------------------------------------------------------
// Filtering proxy callbacks – forward to the game only when menu is closed
// ---------------------------------------------------------------------------

void ProxyCursorPos(GLFWwindow* w, double x, double y) {
  if (g_menu_open && *g_menu_open) { return; }
  if (g_game_cursor_pos) { g_game_cursor_pos(w, x, y); }
}

void ProxyKey(GLFWwindow* w, int key, int scancode, int action, int mods) {
  if (g_menu_open && *g_menu_open) { return; }
  if (g_game_key) { g_game_key(w, key, scancode, action, mods); }
}

void ProxyMouseButton(GLFWwindow* w, int button, int action, int mods) {
  if (g_menu_open && *g_menu_open) { return; }
  if (g_game_mouse_button) { g_game_mouse_button(w, button, action, mods); }
}

void ProxyChar(GLFWwindow* w, unsigned int codepoint) {
  if (g_menu_open && *g_menu_open) { return; }
  if (g_game_char) { g_game_char(w, codepoint); }
}

void ProxyScroll(GLFWwindow* w, double xoff, double yoff) {
  if (g_menu_open && *g_menu_open) { return; }
  if (g_game_scroll) { g_game_scroll(w, xoff, yoff); }
}

// ---------------------------------------------------------------------------
// Callback-setter hooks – intercept (re-)registration so our proxies stay
// in place even when LWJGL re-registers its callbacks.
// ---------------------------------------------------------------------------

GLFWcursorposfun HkSetCursorPosCallback(GLFWwindow* w, GLFWcursorposfun cb) {
  GLFWcursorposfun prev = g_game_cursor_pos;
  g_game_cursor_pos = cb;
  g_orig_set_cursor_pos_cb(w, cb ? &ProxyCursorPos : nullptr);
  return prev;
}

GLFWkeyfun HkSetKeyCallback(GLFWwindow* w, GLFWkeyfun cb) {
  GLFWkeyfun prev = g_game_key;
  g_game_key = cb;
  g_orig_set_key_cb(w, cb ? &ProxyKey : nullptr);
  return prev;
}

GLFWmousebuttonfun HkSetMouseButtonCallback(GLFWwindow* w, GLFWmousebuttonfun cb) {
  GLFWmousebuttonfun prev = g_game_mouse_button;
  g_game_mouse_button = cb;
  g_orig_set_mouse_button_cb(w, cb ? &ProxyMouseButton : nullptr);
  return prev;
}

GLFWcharfun HkSetCharCallback(GLFWwindow* w, GLFWcharfun cb) {
  GLFWcharfun prev = g_game_char;
  g_game_char = cb;
  g_orig_set_char_cb(w, cb ? &ProxyChar : nullptr);
  return prev;
}

GLFWscrollfun HkSetScrollCallback(GLFWwindow* w, GLFWscrollfun cb) {
  GLFWscrollfun prev = g_game_scroll;
  g_game_scroll = cb;
  g_orig_set_scroll_cb(w, cb ? &ProxyScroll : nullptr);
  return prev;
}

// ---------------------------------------------------------------------------
// Helper — install a single inline hook via libmem
// ---------------------------------------------------------------------------

template <typename FnPtr>
bool HookOne(FnPtr target, FnPtr hook, FnPtr* trampoline, std::string_view name) {
  if (!target) {
    PrintStatus(std::format("input suppression: {} not resolved, skipping", name));
    return false;
  }
  const lm_size_t size = LM_HookCode(reinterpret_cast<lm_address_t>(target),
                                     reinterpret_cast<lm_address_t>(hook),
                                     reinterpret_cast<lm_address_t*>(trampoline));
  if (size == 0 || *trampoline == nullptr) {
    PrintStatus(std::format("input suppression: failed to hook {}", name));
    return false;
  }
  return true;
}

}  // namespace

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

bool InstallInputSuppression(OverlayContext& ctx) {
  g_menu_open = &ctx.show_menu;

  int hooked = 0;
  int total = 0;

  auto try_hook = [&](auto target, auto hook, auto* trampoline, std::string_view name) {
    ++total;
    if (HookOne(target, hook, trampoline, name)) { ++hooked; }
  };

  try_hook(ctx.glfw.set_cursor_pos_callback,
           &HkSetCursorPosCallback,
           &g_orig_set_cursor_pos_cb,
           "glfwSetCursorPosCallback");
  try_hook(ctx.glfw.set_key_callback, &HkSetKeyCallback, &g_orig_set_key_cb, "glfwSetKeyCallback");
  try_hook(ctx.glfw.set_mouse_button_callback,
           &HkSetMouseButtonCallback,
           &g_orig_set_mouse_button_cb,
           "glfwSetMouseButtonCallback");
  try_hook(
      ctx.glfw.set_char_callback, &HkSetCharCallback, &g_orig_set_char_cb, "glfwSetCharCallback");
  try_hook(ctx.glfw.set_scroll_callback,
           &HkSetScrollCallback,
           &g_orig_set_scroll_cb,
           "glfwSetScrollCallback");

  PrintStatus(std::format("input suppression: hooked {}/{} functions", hooked, total));
  return hooked > 0;
}

void CaptureGameCallbacks(GLFWwindow* window) {
  // Atomically swap each existing game callback with our filtering proxy.
  // glfwSetXxxCallback returns the previously registered callback, so there
  // is no window where the callback is null.
  if (g_orig_set_cursor_pos_cb) {
    g_game_cursor_pos = g_orig_set_cursor_pos_cb(window, &ProxyCursorPos);
  }
  if (g_orig_set_key_cb) { g_game_key = g_orig_set_key_cb(window, &ProxyKey); }
  if (g_orig_set_mouse_button_cb) {
    g_game_mouse_button = g_orig_set_mouse_button_cb(window, &ProxyMouseButton);
  }
  if (g_orig_set_char_cb) { g_game_char = g_orig_set_char_cb(window, &ProxyChar); }
  if (g_orig_set_scroll_cb) { g_game_scroll = g_orig_set_scroll_cb(window, &ProxyScroll); }

  PrintStatus("input suppression: captured game callbacks and installed proxies");
}

}  // namespace mc_internal
