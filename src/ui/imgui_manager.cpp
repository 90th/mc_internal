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
