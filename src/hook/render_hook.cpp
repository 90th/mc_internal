#include "mc_internal/hook/render_hook.hpp"

#include <cstdio>
#include <memory>
#include <print>

#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES
#endif
#include <GL/gl.h>
#include <GL/glext.h>
#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif
#include <GLFW/glfw3.h>

#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "libmem/libmem.h"

#include "mc_internal/context.hpp"
#include "mc_internal/hook/glfw_bindings.hpp"
#include "mc_internal/ui/imgui_manager.hpp"
#include "mc_internal/ui/input_handler.hpp"
#include "mc_internal/ui/menu.hpp"
#include "mc_internal/ui/opengl_state.hpp"
#include "mc_internal/utils/logging.hpp"

namespace mc_internal {

namespace {

GlfwSwapBuffersFn g_original_swap_buffers = nullptr;
std::unique_ptr<OverlayContext> g_ctx;

void HkGlfwSwapBuffers(GLFWwindow* window) {
  int display_w = 0;
  int display_h = 0;
  int window_w = 0;
  int window_h = 0;

  if (g_ctx->glfw.get_framebuffer_size != nullptr) {
    g_ctx->glfw.get_framebuffer_size(window, &display_w, &display_h);
  }

  if (g_ctx->glfw.get_window_size != nullptr) {
    g_ctx->glfw.get_window_size(window, &window_w, &window_h);
  } else {
    window_w = display_w;
    window_h = display_h;
  }

  if (display_w < 100 || display_h < 100) {
    if (g_original_swap_buffers != nullptr) { g_original_swap_buffers(window); }
    return;
  }

  if (g_ctx->pinned_window == nullptr) {
    g_ctx->pinned_window = window;
    std::println("{} pinned overlay window {}", kLogPrefix, static_cast<const void*>(window));
    std::fflush(stdout);
  }

  if (window != g_ctx->pinned_window) {
    if (g_original_swap_buffers != nullptr) { g_original_swap_buffers(window); }
    return;
  }

  g_ctx->display_width = display_w;
  g_ctx->display_height = display_h;
  g_ctx->window_width = window_w;
  g_ctx->window_height = window_h;

  if (!EnsureImGuiInitialized(*g_ctx)) {
    if (g_original_swap_buffers != nullptr) { g_original_swap_buffers(window); }
    return;
  }

  ImGuiIO& io = ImGui::GetIO();
  io.DisplaySize = ImVec2(static_cast<float>(window_w), static_cast<float>(window_h));
  if (window_w > 0 && window_h > 0) {
    io.DisplayFramebufferScale =
        ImVec2(static_cast<float>(display_w) / static_cast<float>(window_w),
               static_cast<float>(display_h) / static_cast<float>(window_h));
  }
  io.DeltaTime = 1.0f / 60.0f;

  ResetOpenGlStateForImGuiBootstrap();
  ImGui_ImplOpenGL3_NewFrame();
  ImGui::NewFrame();

  ProcessInput(window, *g_ctx);
  RenderMenu(window, *g_ctx);

  ImGui::Render();

  GLint last_fbo = 0;
  glGetIntegerv(GL_FRAMEBUFFER_BINDING, &last_fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  glDisable(GL_CULL_FACE);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_SCISSOR_TEST);
  glDisable(GL_PRIMITIVE_RESTART);

  glEnable(GL_BLEND);
  glBlendEquation(GL_FUNC_ADD);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

  glViewport(0, 0, display_w, display_h);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, 0);
  glBindSampler(0, 0);

  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
  glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(last_fbo));

  if (g_original_swap_buffers != nullptr) { g_original_swap_buffers(window); }
}

}  // namespace

std::expected<void, BootstrapError> InstallRenderHook() {
  auto lookup = LocateGlfwInLwjgl();
  if (lookup.swap_buffers_address == nullptr) {
    return std::unexpected(BootstrapError::kSwapBuffersLookupFailed);
  }

  static_assert(sizeof(GlfwSwapBuffersFn) == sizeof(lm_address_t));

  const lm_size_t hook_size =
      LM_HookCode(reinterpret_cast<lm_address_t>(lookup.swap_buffers_address),
                  reinterpret_cast<lm_address_t>(&HkGlfwSwapBuffers),
                  reinterpret_cast<lm_address_t*>(&g_original_swap_buffers));

  if (hook_size == 0 || g_original_swap_buffers == nullptr) {
    return std::unexpected(BootstrapError::kSwapBuffersHookFailed);
  }

  g_ctx = std::make_unique<OverlayContext>();
  g_ctx->glfw = std::move(lookup.functions);

  return {};
}

}  // namespace mc_internal
