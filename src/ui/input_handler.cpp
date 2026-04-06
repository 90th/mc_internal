#include "mc_internal/ui/input_handler.hpp"

#include <cfloat>

#include "imgui.h"

#include "mc_internal/utils/logging.hpp"

namespace mc_internal {

void ProcessInput(GLFWwindow* window, OverlayContext& ctx) {
  const GlfwFunctions& glfw = ctx.glfw;
  if (glfw.get_key == nullptr || glfw.get_mouse_button == nullptr ||
      glfw.get_cursor_pos == nullptr) {
    return;
  }

  // Suppress GLFW error callback during our GLFW calls to prevent
  // transient GLFW_NOT_INITIALIZED errors on early frames.
  GLFWerrorfun prev_error_callback = nullptr;
  if (glfw.set_error_callback) { prev_error_callback = glfw.set_error_callback(nullptr); }

  ImGuiIO& io = ImGui::GetIO();

  const bool insert_is_down = glfw.get_key(window, GLFW_KEY_INSERT) == GLFW_PRESS;
  if (insert_is_down && !ctx.insert_was_down) {
    ctx.show_menu = !ctx.show_menu;

    if (ctx.show_menu) {
      if (glfw.get_input_mode != nullptr) {
        ctx.original_cursor_mode = glfw.get_input_mode(window, GLFW_CURSOR);
      }

      // Hidden cursor mode breaks the game's camera lock so ImGui gets the
      // real pointer, while keeping the OS cursor invisible (the overlay
      // draws its own custom cursor).
      if (glfw.set_input_mode != nullptr) {
        glfw.set_input_mode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
      }

      io.AddMouseButtonEvent(0, false);
      io.AddMouseButtonEvent(1, false);
    } else {
      if (glfw.set_input_mode != nullptr) {
        glfw.set_input_mode(window, GLFW_CURSOR, ctx.original_cursor_mode);
      }

      // clear any stale imgui mouse state when the menu releases focus.
      io.AddMousePosEvent(-FLT_MAX, -FLT_MAX);
      io.AddMouseButtonEvent(0, false);
      io.AddMouseButtonEvent(1, false);
    }

    PrintStatus(ctx.show_menu ? "menu toggled open" : "menu toggled closed");
  }
  ctx.insert_was_down = insert_is_down;

  if (ctx.show_menu) {
    // Per-frame override: prevent the game from re-grabbing the cursor while
    // the menu is open (replaces the removed glfwSetInputMode inline hook).
    if (glfw.set_input_mode != nullptr) {
      glfw.set_input_mode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    }

    double mouse_x = 0.0;
    double mouse_y = 0.0;
    glfw.get_cursor_pos(window, &mouse_x, &mouse_y);

    io.AddMousePosEvent(static_cast<float>(mouse_x), static_cast<float>(mouse_y));
    io.AddMouseButtonEvent(0, glfw.get_mouse_button(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS);
    io.AddMouseButtonEvent(1, glfw.get_mouse_button(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS);
  }

  if (glfw.set_error_callback && prev_error_callback) {
    glfw.set_error_callback(prev_error_callback);
  }
}

}  // namespace mc_internal
