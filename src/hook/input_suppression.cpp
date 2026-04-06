#include "mc_internal/hook/input_suppression.hpp"

#include <array>
#include <cstdint>
#include <cstring>
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

constexpr std::size_t kHookStubSize = 14;

struct InlineHookState {
  void* target = nullptr;
  std::array<unsigned char, kHookStubSize> original_bytes{};
  std::array<unsigned char, kHookStubSize> hook_bytes{};
};

// ---------------------------------------------------------------------------
// Shared suppression state
// ---------------------------------------------------------------------------

const bool* g_menu_open = nullptr;

// ---------------------------------------------------------------------------
// Per-function inline hook states
// ---------------------------------------------------------------------------

InlineHookState g_hk_set_cursor_pos_cb{};
InlineHookState g_hk_set_key_cb{};
InlineHookState g_hk_set_mouse_button_cb{};
InlineHookState g_hk_set_char_cb{};
InlineHookState g_hk_set_scroll_cb{};

// ---------------------------------------------------------------------------
// Stored game callbacks (captured at init or when the game re-registers)
// ---------------------------------------------------------------------------

GLFWcursorposfun g_game_cursor_pos = nullptr;
GLFWkeyfun g_game_key = nullptr;
GLFWmousebuttonfun g_game_mouse_button = nullptr;
GLFWcharfun g_game_char = nullptr;
GLFWscrollfun g_game_scroll = nullptr;

// ---------------------------------------------------------------------------
// Inline hook helpers
// ---------------------------------------------------------------------------

void WriteBytes(InlineHookState& state, const unsigned char* bytes) {
  lm_prot_t old_prot{};
  auto addr = reinterpret_cast<lm_address_t>(state.target);
  LM_ProtMemory(addr, kHookStubSize, LM_PROT_XRW, &old_prot);
  std::memcpy(state.target, bytes, kHookStubSize);
  LM_ProtMemory(addr, kHookStubSize, old_prot, nullptr);
}

bool InstallInlineHook(InlineHookState& state, void* target, void* hook) {
  state.target = target;
  std::memcpy(state.original_bytes.data(), target, kHookStubSize);

  state.hook_bytes[0] = 0xFF;
  state.hook_bytes[1] = 0x25;
  state.hook_bytes[2] = 0x00;
  state.hook_bytes[3] = 0x00;
  state.hook_bytes[4] = 0x00;
  state.hook_bytes[5] = 0x00;
  auto hook_addr = reinterpret_cast<std::uintptr_t>(hook);
  std::memcpy(&state.hook_bytes[6], &hook_addr, 8);

  WriteBytes(state, state.hook_bytes.data());
  return true;
}

template <typename Fn, typename... Args>
auto CallReal(InlineHookState& state, Args&&... args) {
  WriteBytes(state, state.original_bytes.data());
  auto result = reinterpret_cast<Fn>(state.target)(std::forward<Args>(args)...);
  WriteBytes(state, state.hook_bytes.data());
  return result;
}

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
  CallReal<GlfwSetCursorPosCallbackFn>(g_hk_set_cursor_pos_cb, w, cb ? &ProxyCursorPos : nullptr);
  return prev;
}

GLFWkeyfun HkSetKeyCallback(GLFWwindow* w, GLFWkeyfun cb) {
  GLFWkeyfun prev = g_game_key;
  g_game_key = cb;
  CallReal<GlfwSetKeyCallbackFn>(g_hk_set_key_cb, w, cb ? &ProxyKey : nullptr);
  return prev;
}

GLFWmousebuttonfun HkSetMouseButtonCallback(GLFWwindow* w, GLFWmousebuttonfun cb) {
  GLFWmousebuttonfun prev = g_game_mouse_button;
  g_game_mouse_button = cb;
  CallReal<GlfwSetMouseButtonCallbackFn>(
      g_hk_set_mouse_button_cb, w, cb ? &ProxyMouseButton : nullptr);
  return prev;
}

GLFWcharfun HkSetCharCallback(GLFWwindow* w, GLFWcharfun cb) {
  GLFWcharfun prev = g_game_char;
  g_game_char = cb;
  CallReal<GlfwSetCharCallbackFn>(g_hk_set_char_cb, w, cb ? &ProxyChar : nullptr);
  return prev;
}

GLFWscrollfun HkSetScrollCallback(GLFWwindow* w, GLFWscrollfun cb) {
  GLFWscrollfun prev = g_game_scroll;
  g_game_scroll = cb;
  CallReal<GlfwSetScrollCallbackFn>(g_hk_set_scroll_cb, w, cb ? &ProxyScroll : nullptr);
  return prev;
}

}  // namespace

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

bool InstallInputSuppression(OverlayContext& ctx) {
  g_menu_open = &ctx.show_menu;

  int hooked = 0;
  int total = 0;

  auto try_hook = [&](auto target, auto hook, InlineHookState& state, std::string_view name) {
    ++total;
    if (!target) {
      PrintStatus(std::format("input suppression: {} not resolved, skipping", name));
      return;
    }
    if (InstallInlineHook(state, reinterpret_cast<void*>(target), reinterpret_cast<void*>(hook))) {
      ++hooked;
    }
  };

  try_hook(ctx.glfw.set_cursor_pos_callback,
           &HkSetCursorPosCallback,
           g_hk_set_cursor_pos_cb,
           "glfwSetCursorPosCallback");
  try_hook(ctx.glfw.set_key_callback, &HkSetKeyCallback, g_hk_set_key_cb, "glfwSetKeyCallback");
  try_hook(ctx.glfw.set_mouse_button_callback,
           &HkSetMouseButtonCallback,
           g_hk_set_mouse_button_cb,
           "glfwSetMouseButtonCallback");
  try_hook(ctx.glfw.set_char_callback, &HkSetCharCallback, g_hk_set_char_cb, "glfwSetCharCallback");
  try_hook(ctx.glfw.set_scroll_callback,
           &HkSetScrollCallback,
           g_hk_set_scroll_cb,
           "glfwSetScrollCallback");

  PrintStatus(std::format("input suppression: hooked {}/{} functions", hooked, total));
  return hooked > 0;
}

void CaptureGameCallbacks(GLFWwindow* window) {
  if (g_hk_set_cursor_pos_cb.target) {
    g_game_cursor_pos =
        CallReal<GlfwSetCursorPosCallbackFn>(g_hk_set_cursor_pos_cb, window, &ProxyCursorPos);
  }
  if (g_hk_set_key_cb.target) {
    g_game_key = CallReal<GlfwSetKeyCallbackFn>(g_hk_set_key_cb, window, &ProxyKey);
  }
  if (g_hk_set_mouse_button_cb.target) {
    g_game_mouse_button =
        CallReal<GlfwSetMouseButtonCallbackFn>(g_hk_set_mouse_button_cb, window, &ProxyMouseButton);
  }
  if (g_hk_set_char_cb.target) {
    g_game_char = CallReal<GlfwSetCharCallbackFn>(g_hk_set_char_cb, window, &ProxyChar);
  }
  if (g_hk_set_scroll_cb.target) {
    g_game_scroll = CallReal<GlfwSetScrollCallbackFn>(g_hk_set_scroll_cb, window, &ProxyScroll);
  }

  PrintStatus("input suppression: captured game callbacks and installed proxies");
}

}  // namespace mc_internal
