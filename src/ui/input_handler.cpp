#include "mc_internal/ui/input_handler.hpp"

#include "imgui.h"

#include "mc_internal/utils/logging.hpp"

namespace mc_internal {

void ProcessInput(GLFWwindow* window, OverlayContext& ctx) {
  const GlfwFunctions& glfw = ctx.glfw;
  if (glfw.get_key == nullptr || glfw.get_mouse_button == nullptr ||
      glfw.get_cursor_pos == nullptr) {
    return;
  }

  const bool insert_is_down = glfw.get_key(window, GLFW_KEY_INSERT) == GLFW_PRESS;
  if (insert_is_down && !ctx.insert_was_down) {
    ctx.show_menu = !ctx.show_menu;

    if (ctx.show_menu) {
      if (glfw.get_input_mode != nullptr) {
        ctx.original_cursor_mode = glfw.get_input_mode(window, GLFW_CURSOR);
      }
      // FIX: Only hide the OS cursor ONCE when the menu opens, preventing Wayland spam.
      if (glfw.set_input_mode != nullptr) {
        glfw.set_input_mode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
      }
    } else {
      // Restore the game's cursor mode ONCE when the menu closes.
      if (glfw.set_input_mode != nullptr) {
        glfw.set_input_mode(window, GLFW_CURSOR, ctx.original_cursor_mode);
      }
    }

    PrintStatus(ctx.show_menu ? "menu toggled open" : "menu toggled closed");
  }
  ctx.insert_was_down = insert_is_down;

  if (ctx.show_menu) {
    double mouse_x = 0.0;
    double mouse_y = 0.0;
    glfw.get_cursor_pos(window, &mouse_x, &mouse_y);

    ImGuiIO& io = ImGui::GetIO();
    io.AddMousePosEvent(static_cast<float>(mouse_x), static_cast<float>(mouse_y));
    io.AddMouseButtonEvent(0, glfw.get_mouse_button(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS);
    io.AddMouseButtonEvent(1, glfw.get_mouse_button(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS);
  }
}

}  // namespace mc_internal