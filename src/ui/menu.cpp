#include "mc_internal/ui/menu.hpp"

#include <algorithm>

#include "imgui.h"

namespace mc_internal {

void RenderMenu(GLFWwindow* window, const OverlayContext& ctx) {
  if (ctx.show_menu) {
    ImGui::SetNextWindowPos(ImVec2(40.0f, 40.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(
        ImVec2(std::max(360.0f, static_cast<float>(ctx.window_width) * 0.35f),
               std::max(180.0f, static_cast<float>(ctx.window_height) * 0.25f)),
        ImGuiCond_FirstUseEver);

    ImGui::Begin("mc internal");
    ImGui::Text("opengl hook active");
    ImGui::Text("pinned window: %p", static_cast<const void*>(window));
    ImGui::Text("logical window: %d x %d", ctx.window_width, ctx.window_height);
    ImGui::Text("framebuffer: %d x %d", ctx.display_width, ctx.display_height);
    ImGui::End();
  }

  ctx.module_manager.RenderUi(ctx);

  if (!ctx.show_menu) { return; }

  // draw a custom dark red cursor on top of everything
  ImDrawList* fg = ImGui::GetForegroundDrawList();
  const ImVec2 m = ImGui::GetIO().MousePos;
  const ImU32 cursor_color = IM_COL32(180, 20, 20, 255);
  const ImU32 border_color = IM_COL32(0, 0, 0, 255);

  // border first for visibility
  fg->AddTriangle(
      m, ImVec2(m.x + 13.0f, m.y + 13.0f), ImVec2(m.x, m.y + 18.0f), border_color, 1.5f);

  // inner red fill
  fg->AddTriangleFilled(
      m, ImVec2(m.x + 13.0f, m.y + 13.0f), ImVec2(m.x, m.y + 18.0f), cursor_color);
}

}  // namespace mc_internal
