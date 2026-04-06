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

#include "imgui.h"
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
// GLFW key → ImGuiKey translation (mirrors imgui_impl_glfw.cpp)
// ---------------------------------------------------------------------------

ImGuiKey GlfwKeyToImGui(int key) {
  switch (key) {
    case GLFW_KEY_TAB:
      return ImGuiKey_Tab;
    case GLFW_KEY_LEFT:
      return ImGuiKey_LeftArrow;
    case GLFW_KEY_RIGHT:
      return ImGuiKey_RightArrow;
    case GLFW_KEY_UP:
      return ImGuiKey_UpArrow;
    case GLFW_KEY_DOWN:
      return ImGuiKey_DownArrow;
    case GLFW_KEY_PAGE_UP:
      return ImGuiKey_PageUp;
    case GLFW_KEY_PAGE_DOWN:
      return ImGuiKey_PageDown;
    case GLFW_KEY_HOME:
      return ImGuiKey_Home;
    case GLFW_KEY_END:
      return ImGuiKey_End;
    case GLFW_KEY_INSERT:
      return ImGuiKey_Insert;
    case GLFW_KEY_DELETE:
      return ImGuiKey_Delete;
    case GLFW_KEY_BACKSPACE:
      return ImGuiKey_Backspace;
    case GLFW_KEY_SPACE:
      return ImGuiKey_Space;
    case GLFW_KEY_ENTER:
      return ImGuiKey_Enter;
    case GLFW_KEY_ESCAPE:
      return ImGuiKey_Escape;
    case GLFW_KEY_APOSTROPHE:
      return ImGuiKey_Apostrophe;
    case GLFW_KEY_COMMA:
      return ImGuiKey_Comma;
    case GLFW_KEY_MINUS:
      return ImGuiKey_Minus;
    case GLFW_KEY_PERIOD:
      return ImGuiKey_Period;
    case GLFW_KEY_SLASH:
      return ImGuiKey_Slash;
    case GLFW_KEY_SEMICOLON:
      return ImGuiKey_Semicolon;
    case GLFW_KEY_EQUAL:
      return ImGuiKey_Equal;
    case GLFW_KEY_LEFT_BRACKET:
      return ImGuiKey_LeftBracket;
    case GLFW_KEY_BACKSLASH:
      return ImGuiKey_Backslash;
    case GLFW_KEY_RIGHT_BRACKET:
      return ImGuiKey_RightBracket;
    case GLFW_KEY_GRAVE_ACCENT:
      return ImGuiKey_GraveAccent;
    case GLFW_KEY_CAPS_LOCK:
      return ImGuiKey_CapsLock;
    case GLFW_KEY_SCROLL_LOCK:
      return ImGuiKey_ScrollLock;
    case GLFW_KEY_NUM_LOCK:
      return ImGuiKey_NumLock;
    case GLFW_KEY_PRINT_SCREEN:
      return ImGuiKey_PrintScreen;
    case GLFW_KEY_PAUSE:
      return ImGuiKey_Pause;
    case GLFW_KEY_KP_0:
      return ImGuiKey_Keypad0;
    case GLFW_KEY_KP_1:
      return ImGuiKey_Keypad1;
    case GLFW_KEY_KP_2:
      return ImGuiKey_Keypad2;
    case GLFW_KEY_KP_3:
      return ImGuiKey_Keypad3;
    case GLFW_KEY_KP_4:
      return ImGuiKey_Keypad4;
    case GLFW_KEY_KP_5:
      return ImGuiKey_Keypad5;
    case GLFW_KEY_KP_6:
      return ImGuiKey_Keypad6;
    case GLFW_KEY_KP_7:
      return ImGuiKey_Keypad7;
    case GLFW_KEY_KP_8:
      return ImGuiKey_Keypad8;
    case GLFW_KEY_KP_9:
      return ImGuiKey_Keypad9;
    case GLFW_KEY_KP_DECIMAL:
      return ImGuiKey_KeypadDecimal;
    case GLFW_KEY_KP_DIVIDE:
      return ImGuiKey_KeypadDivide;
    case GLFW_KEY_KP_MULTIPLY:
      return ImGuiKey_KeypadMultiply;
    case GLFW_KEY_KP_SUBTRACT:
      return ImGuiKey_KeypadSubtract;
    case GLFW_KEY_KP_ADD:
      return ImGuiKey_KeypadAdd;
    case GLFW_KEY_KP_ENTER:
      return ImGuiKey_KeypadEnter;
    case GLFW_KEY_KP_EQUAL:
      return ImGuiKey_KeypadEqual;
    case GLFW_KEY_LEFT_SHIFT:
      return ImGuiKey_LeftShift;
    case GLFW_KEY_LEFT_CONTROL:
      return ImGuiKey_LeftCtrl;
    case GLFW_KEY_LEFT_ALT:
      return ImGuiKey_LeftAlt;
    case GLFW_KEY_LEFT_SUPER:
      return ImGuiKey_LeftSuper;
    case GLFW_KEY_RIGHT_SHIFT:
      return ImGuiKey_RightShift;
    case GLFW_KEY_RIGHT_CONTROL:
      return ImGuiKey_RightCtrl;
    case GLFW_KEY_RIGHT_ALT:
      return ImGuiKey_RightAlt;
    case GLFW_KEY_RIGHT_SUPER:
      return ImGuiKey_RightSuper;
    case GLFW_KEY_MENU:
      return ImGuiKey_Menu;
    default:
      if (key >= GLFW_KEY_0 && key <= GLFW_KEY_9)
        return static_cast<ImGuiKey>(ImGuiKey_0 + (key - GLFW_KEY_0));
      if (key >= GLFW_KEY_A && key <= GLFW_KEY_Z)
        return static_cast<ImGuiKey>(ImGuiKey_A + (key - GLFW_KEY_A));
      if (key >= GLFW_KEY_F1 && key <= GLFW_KEY_F24)
        return static_cast<ImGuiKey>(ImGuiKey_F1 + (key - GLFW_KEY_F1));
      return ImGuiKey_None;
  }
}

void UpdateImGuiModifiers(int mods) {
  ImGuiIO& io = ImGui::GetIO();
  io.AddKeyEvent(ImGuiMod_Ctrl, (mods & GLFW_MOD_CONTROL) != 0);
  io.AddKeyEvent(ImGuiMod_Shift, (mods & GLFW_MOD_SHIFT) != 0);
  io.AddKeyEvent(ImGuiMod_Alt, (mods & GLFW_MOD_ALT) != 0);
  io.AddKeyEvent(ImGuiMod_Super, (mods & GLFW_MOD_SUPER) != 0);
}

// ---------------------------------------------------------------------------
// Filtering proxy callbacks – forward to the game only when menu is closed,
// forward to ImGui when menu is open.
// ---------------------------------------------------------------------------

void ProxyCursorPos(GLFWwindow* w, double x, double y) {
  if (g_menu_open && *g_menu_open) { return; }
  if (g_game_cursor_pos) { g_game_cursor_pos(w, x, y); }
}

void ProxyKey(GLFWwindow* w, int key, int scancode, int action, int mods) {
  if (g_menu_open && *g_menu_open) {
    UpdateImGuiModifiers(mods);
    ImGuiKey imgui_key = GlfwKeyToImGui(key);
    if (imgui_key != ImGuiKey_None) {
      ImGui::GetIO().AddKeyEvent(imgui_key, action != GLFW_RELEASE);
    }
    return;
  }
  if (g_game_key) { g_game_key(w, key, scancode, action, mods); }
}

void ProxyMouseButton(GLFWwindow* w, int button, int action, int mods) {
  if (g_menu_open && *g_menu_open) { return; }
  if (g_game_mouse_button) { g_game_mouse_button(w, button, action, mods); }
}

void ProxyChar(GLFWwindow* w, unsigned int codepoint) {
  if (g_menu_open && *g_menu_open) {
    ImGui::GetIO().AddInputCharacter(codepoint);
    return;
  }
  if (g_game_char) { g_game_char(w, codepoint); }
}

void ProxyScroll(GLFWwindow* w, double xoff, double yoff) {
  if (g_menu_open && *g_menu_open) {
    ImGui::GetIO().AddMouseWheelEvent(static_cast<float>(xoff), static_cast<float>(yoff));
    return;
  }
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
