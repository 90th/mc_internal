#include "mc_internal/ui/imgui_manager.hpp"

#include <cstdio>
#include <cstring>
#include <print>
#include <string_view>

#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES
#endif
#include <GL/gl.h>
#include <GL/glext.h>

#include "imgui.h"
#include "imgui_impl_opengl3.h"

#include "mc_internal/ui/opengl_state.hpp"
#include "mc_internal/utils/logging.hpp"

namespace mc_internal {

namespace {

constexpr std::string_view kDefaultGlslVersion = "#version 150";

constexpr const char* kFontPath = MCINTERNAL_FONT_PATH;

void ApplyTheme() {
  ImGuiStyle& style = ImGui::GetStyle();

  // ── geometry ─────────────────────────────────────────────────
  style.Alpha = 1.0f;
  style.DisabledAlpha = 0.5f;
  style.WindowPadding = ImVec2(12.0f, 12.0f);
  style.WindowRounding = 6.0f;
  style.WindowBorderSize = 1.0f;
  style.WindowMinSize = ImVec2(32.0f, 32.0f);
  style.WindowTitleAlign = ImVec2(0.0f, 0.5f);
  style.WindowMenuButtonPosition = ImGuiDir_None;
  style.ChildRounding = 4.0f;
  style.ChildBorderSize = 1.0f;
  style.PopupRounding = 4.0f;
  style.PopupBorderSize = 1.0f;
  style.FramePadding = ImVec2(8.0f, 5.0f);
  style.FrameRounding = 4.0f;
  style.FrameBorderSize = 0.0f;
  style.ItemSpacing = ImVec2(8.0f, 6.0f);
  style.ItemInnerSpacing = ImVec2(6.0f, 4.0f);
  style.CellPadding = ImVec2(4.0f, 2.0f);
  style.IndentSpacing = 16.0f;
  style.ColumnsMinSpacing = 6.0f;
  style.ScrollbarSize = 10.0f;
  style.ScrollbarRounding = 4.0f;
  style.GrabMinSize = 8.0f;
  style.GrabRounding = 3.0f;
  style.TabRounding = 4.0f;
  style.TabBorderSize = 0.0f;
  style.ColorButtonPosition = ImGuiDir_Right;
  style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
  style.SelectableTextAlign = ImVec2(0.0f, 0.0f);
  style.SeparatorTextPadding = ImVec2(8.0f, 2.0f);

  // ── palette ──────────────────────────────────────────────────
  auto& c = style.Colors;
  c[ImGuiCol_Text] = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
  c[ImGuiCol_TextDisabled] = ImVec4(0.42f, 0.42f, 0.42f, 1.00f);
  c[ImGuiCol_WindowBg] = ImVec4(0.094f, 0.098f, 0.110f, 0.97f);
  c[ImGuiCol_ChildBg] = ImVec4(0.110f, 0.114f, 0.126f, 1.00f);
  c[ImGuiCol_PopupBg] = ImVec4(0.110f, 0.114f, 0.126f, 0.97f);
  c[ImGuiCol_Border] = ImVec4(0.18f, 0.18f, 0.22f, 1.00f);
  c[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
  c[ImGuiCol_FrameBg] = ImVec4(0.13f, 0.14f, 0.16f, 1.00f);
  c[ImGuiCol_FrameBgHovered] = ImVec4(0.16f, 0.17f, 0.19f, 1.00f);
  c[ImGuiCol_FrameBgActive] = ImVec4(0.18f, 0.19f, 0.21f, 1.00f);
  c[ImGuiCol_TitleBg] = ImVec4(0.094f, 0.098f, 0.110f, 1.00f);
  c[ImGuiCol_TitleBgActive] = ImVec4(0.094f, 0.098f, 0.110f, 1.00f);
  c[ImGuiCol_TitleBgCollapsed] = ImVec4(0.094f, 0.098f, 0.110f, 0.80f);
  c[ImGuiCol_MenuBarBg] = ImVec4(0.12f, 0.13f, 0.14f, 1.00f);
  c[ImGuiCol_ScrollbarBg] = ImVec4(0.094f, 0.098f, 0.110f, 0.40f);
  c[ImGuiCol_ScrollbarGrab] = ImVec4(0.24f, 0.24f, 0.28f, 1.00f);
  c[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.32f, 0.32f, 0.36f, 1.00f);
  c[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.40f, 0.40f, 0.44f, 1.00f);

  const ImVec4 accent(0.78f, 0.24f, 0.24f, 1.00f);
  const ImVec4 accent_hover(0.88f, 0.30f, 0.30f, 1.00f);
  const ImVec4 accent_active(0.68f, 0.20f, 0.20f, 1.00f);

  c[ImGuiCol_CheckMark] = accent;
  c[ImGuiCol_SliderGrab] = ImVec4(0.78f, 0.78f, 0.78f, 1.00f);
  c[ImGuiCol_SliderGrabActive] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
  c[ImGuiCol_Button] = ImVec4(0.15f, 0.16f, 0.18f, 1.00f);
  c[ImGuiCol_ButtonHovered] = ImVec4(0.20f, 0.21f, 0.24f, 1.00f);
  c[ImGuiCol_ButtonActive] = ImVec4(0.25f, 0.26f, 0.30f, 1.00f);
  c[ImGuiCol_Header] = ImVec4(0.14f, 0.15f, 0.17f, 1.00f);
  c[ImGuiCol_HeaderHovered] = ImVec4(0.18f, 0.19f, 0.22f, 1.00f);
  c[ImGuiCol_HeaderActive] = ImVec4(0.22f, 0.23f, 0.26f, 1.00f);
  c[ImGuiCol_Separator] = ImVec4(0.20f, 0.20f, 0.24f, 0.50f);
  c[ImGuiCol_SeparatorHovered] = accent_hover;
  c[ImGuiCol_SeparatorActive] = accent;
  c[ImGuiCol_ResizeGrip] = ImVec4(0.20f, 0.20f, 0.24f, 0.20f);
  c[ImGuiCol_ResizeGripHovered] = ImVec4(0.30f, 0.30f, 0.34f, 0.40f);
  c[ImGuiCol_ResizeGripActive] = ImVec4(0.40f, 0.40f, 0.44f, 0.60f);
  c[ImGuiCol_Tab] = ImVec4(0.13f, 0.14f, 0.16f, 1.00f);
  c[ImGuiCol_TabHovered] = accent_hover;
  c[ImGuiCol_TabActive] = accent;
  c[ImGuiCol_TabUnfocused] = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
  c[ImGuiCol_TabUnfocusedActive] = ImVec4(0.15f, 0.15f, 0.18f, 1.00f);
  c[ImGuiCol_PlotLines] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
  c[ImGuiCol_PlotLinesHovered] = accent_hover;
  c[ImGuiCol_PlotHistogram] = accent;
  c[ImGuiCol_PlotHistogramHovered] = accent_hover;
  c[ImGuiCol_TableHeaderBg] = ImVec4(0.12f, 0.13f, 0.15f, 1.00f);
  c[ImGuiCol_TableBorderStrong] = ImVec4(0.20f, 0.20f, 0.24f, 1.00f);
  c[ImGuiCol_TableBorderLight] = ImVec4(0.16f, 0.16f, 0.20f, 1.00f);
  c[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
  c[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.03f);
  c[ImGuiCol_TextSelectedBg] = ImVec4(accent.x, accent.y, accent.z, 0.30f);
  c[ImGuiCol_DragDropTarget] = accent;
  c[ImGuiCol_NavHighlight] = accent;
  c[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.60f);
  c[ImGuiCol_NavWindowingDimBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.50f);
  c[ImGuiCol_ModalWindowDimBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.55f);
}

class ScopedImGuiBootstrapState {
 public:
  ScopedImGuiBootstrapState() : state_(PrepareOpenGlStateForImGuiBootstrap()) {}
  ~ScopedImGuiBootstrapState() { RestoreOpenGlStateAfterImGuiBootstrap(state_); }

  ScopedImGuiBootstrapState(const ScopedImGuiBootstrapState&) = delete;
  ScopedImGuiBootstrapState& operator=(const ScopedImGuiBootstrapState&) = delete;

 private:
  OpenGlBootstrapState state_{};
};

}  // namespace

bool EnsureImGuiInitialized(OverlayContext& ctx) {
  if (ctx.imgui_initialized) { return true; }
  if (ctx.imgui_init_failed) { return false; }

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();

  ImGuiIO& io = ImGui::GetIO();
  io.IniFilename = nullptr;
  io.MouseDrawCursor = false;
  io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
  io.SetClipboardTextFn = nullptr;
  io.GetClipboardTextFn = nullptr;

  ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
  platform_io.Platform_SetClipboardTextFn = nullptr;
  platform_io.Platform_GetClipboardTextFn = nullptr;
  platform_io.Platform_SetImeDataFn = nullptr;

  if (kFontPath != nullptr && kFontPath[0] != '\0') {
    ImFontConfig font_cfg;
    font_cfg.OversampleH = 2;
    font_cfg.OversampleV = 1;
    font_cfg.PixelSnapH = true;
    io.Fonts->AddFontFromFileTTF(kFontPath, 14.0f, &font_cfg);
    PrintStatus("loaded custom font from deps/imgui/misc/fonts");
  }

  if (!ImGui_ImplOpenGL3_Init(kDefaultGlslVersion.data())) {
    ctx.imgui_init_failed = true;
    ImGui::DestroyContext();
    PrintStatus("imgui initialization failed on the render thread");
    return false;
  }

  {
    ScopedImGuiBootstrapState scoped_bootstrap_state;
    if (!ImGui_ImplOpenGL3_CreateDeviceObjects()) {
      ctx.imgui_init_failed = true;
      ImGui_ImplOpenGL3_Shutdown();
      ImGui::DestroyContext();
      PrintStatus("imgui device object creation failed on the render thread");
      return false;
    }
  }

  ApplyTheme();

  ctx.imgui_initialized = true;
  PrintStatus("imgui initialized purely via opengl");
  return true;
}

}  // namespace mc_internal
