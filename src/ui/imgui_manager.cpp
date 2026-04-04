#include "mc_internal/ui/imgui_manager.hpp"

#include <cstdio>
#include <print>
#include <string_view>

#include "imgui.h"
#include "imgui_impl_opengl3.h"

#include "mc_internal/ui/opengl_state.hpp"
#include "mc_internal/utils/logging.hpp"

namespace mc_internal {

namespace {

constexpr std::string_view kDefaultGlslVersion = "#version 150";

void DarkTheme() {
  ImGuiStyle& style = ImGui::GetStyle();
  style.Alpha = 1.0f;
  style.DisabledAlpha = 0.6f;
  style.WindowPadding = ImVec2(8.0f, 8.0f);
  style.WindowRounding = 0.0f;
  style.WindowBorderSize = 1.0f;
  style.WindowMinSize = ImVec2(32.0f, 32.0f);
  style.WindowTitleAlign = ImVec2(0.0f, 0.5f);
  style.WindowMenuButtonPosition = ImGuiDir_Left;
  style.ChildRounding = 0.0f;
  style.ChildBorderSize = 1.0f;
  style.PopupRounding = 0.0f;
  style.PopupBorderSize = 1.0f;
  style.FramePadding = ImVec2(4.0f, 3.0f);
  style.FrameRounding = 0.0f;
  style.FrameBorderSize = 0.0f;
  style.ItemSpacing = ImVec2(8.0f, 4.0f);
  style.ItemInnerSpacing = ImVec2(4.0f, 4.0f);
  style.CellPadding = ImVec2(4.0f, 2.0f);
  style.IndentSpacing = 21.0f;
  style.ColumnsMinSpacing = 6.0f;
  style.ScrollbarSize = 14.0f;
  style.ScrollbarRounding = 0.0f;
  style.GrabMinSize = 10.0f;
  style.GrabRounding = 0.0f;
  style.TabRounding = 0.0f;
  style.TabBorderSize = 0.0f;
  style.ColorButtonPosition = ImGuiDir_Right;
  style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
  style.SelectableTextAlign = ImVec2(0.0f, 0.0f);

  style.Colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
  style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.49803922f, 0.49803922f, 0.49803922f, 1.0f);
  style.Colors[ImGuiCol_WindowBg] = ImVec4(0.039215688f, 0.039215688f, 0.039215688f, 1.0f);
  style.Colors[ImGuiCol_ChildBg] = ImVec4(0.05490196f, 0.05490196f, 0.05490196f, 1.0f);
  style.Colors[ImGuiCol_PopupBg] = ImVec4(0.078431375f, 0.078431375f, 0.078431375f, 0.8583691f);
  style.Colors[ImGuiCol_Border] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
  style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.6995708f);
  style.Colors[ImGuiCol_FrameBg] = ImVec4(0.05490196f, 0.05490196f, 0.05490196f, 1.0f);
  style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.06666667f, 0.06666667f, 0.06666667f, 1.0f);
  style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.05490196f, 0.05490196f, 0.05490196f, 0.05490196f);
  style.Colors[ImGuiCol_TitleBg] = ImVec4(0.18431373f, 0.19215687f, 0.21176471f, 1.0f);
  style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.039215688f, 0.039215688f, 0.039215688f, 1.0f);
  style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.05490196f, 0.05490196f, 0.05490196f, 1.0f);
  style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.078431375f, 0.078431375f, 0.078431375f, 0.94f);
  style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.019607844f, 0.019607844f, 0.019607844f, 0.53f);
  style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.30980393f, 0.30980393f, 0.30980393f, 1.0f);
  style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.40784314f, 0.40784314f, 0.40784314f, 1.0f);
  style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.50980395f, 0.50980395f, 0.50980395f, 1.0f);
  style.Colors[ImGuiCol_CheckMark] = ImVec4(0.64705884f, 0.23137255f, 0.23137255f, 1.0f);
  style.Colors[ImGuiCol_SliderGrab] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
  style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
  style.Colors[ImGuiCol_Button] = ImVec4(0.05490196f, 0.05490196f, 0.05490196f, 1.0f);
  style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.06666667f, 0.06666667f, 0.06666667f, 1.0f);
  style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.078431375f, 0.078431375f, 0.078431375f, 1.0f);
  style.Colors[ImGuiCol_Header] = ImVec4(0.05490196f, 0.05490196f, 0.05490196f, 1.0f);
  style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.06666667f, 0.06666667f, 0.06666667f, 1.0f);
  style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.078431375f, 0.078431375f, 0.078431375f, 1.0f);
  style.Colors[ImGuiCol_Separator] = ImVec4(0.078431375f, 0.078431375f, 0.078431375f, 0.5019608f);
  style.Colors[ImGuiCol_SeparatorHovered] =
      ImVec4(0.078431375f, 0.078431375f, 0.078431375f, 0.6695279f);
  style.Colors[ImGuiCol_SeparatorActive] =
      ImVec4(0.078431375f, 0.078431375f, 0.078431375f, 0.95708156f);
  style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.101960786f, 0.11372549f, 0.12941177f, 0.2f);
  style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.20392157f, 0.20784314f, 0.21568628f, 0.2f);
  style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.3019608f, 0.3019608f, 0.3019608f, 0.2f);
  style.Colors[ImGuiCol_Tab] = ImVec4(0.18431373f, 0.19215687f, 0.21176471f, 1.0f);
  style.Colors[ImGuiCol_TabHovered] = ImVec4(0.23529412f, 0.24705882f, 0.27058825f, 1.0f);
  style.Colors[ImGuiCol_TabActive] = ImVec4(0.25882354f, 0.27450982f, 0.3019608f, 1.0f);
  style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.06666667f, 0.06666667f, 0.06666667f, 0.972549f);
  style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.06666667f, 0.06666667f, 0.06666667f, 1.0f);
  style.Colors[ImGuiCol_PlotLines] = ImVec4(0.60784316f, 0.60784316f, 0.60784316f, 1.0f);
  style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.9490196f, 0.34509805f, 0.34509805f, 1.0f);
  style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.9490196f, 0.34509805f, 0.34509805f, 1.0f);
  style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.42745098f, 0.36078432f, 0.36078432f, 1.0f);
  style.Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.1882353f, 0.1882353f, 0.2f, 1.0f);
  style.Colors[ImGuiCol_TableBorderStrong] = ImVec4(0.30980393f, 0.30980393f, 0.34901962f, 1.0f);
  style.Colors[ImGuiCol_TableBorderLight] = ImVec4(0.22745098f, 0.22745098f, 0.24705882f, 1.0f);
  style.Colors[ImGuiCol_TableRowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
  style.Colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.0f, 1.0f, 1.0f, 0.06f);
  style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(1.0f, 0.8784314f, 0.8784314f, 1.0f);
  style.Colors[ImGuiCol_DragDropTarget] = ImVec4(0.25882354f, 0.27058825f, 0.38039216f, 1.0f);
  style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.18039216f, 0.22745098f, 0.2784314f, 1.0f);
  style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0f, 1.0f, 1.0f, 0.7f);
  style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.8f, 0.8f, 0.8f, 0.2f);
  style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.8f, 0.8f, 0.8f, 0.35f);
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

  ForceBuildAndLogFontAtlasTexture();
  DarkTheme();

  ctx.imgui_initialized = true;
  PrintStatus("imgui initialized purely via opengl");
  return true;
}

void ForceBuildAndLogFontAtlasTexture() {
  ImGuiIO& io = ImGui::GetIO();

  unsigned char* pixels = nullptr;
  int atlas_width = 0;
  int atlas_height = 0;
  int atlas_bpp = 0;
  io.Fonts->GetTexDataAsRGBA32(&pixels, &atlas_width, &atlas_height, &atlas_bpp);

  std::println(
      "{} font atlas prepared ({}x{}x{})", kLogPrefix, atlas_width, atlas_height, atlas_bpp);
  std::fflush(stdout);
}

}  // namespace mc_internal
