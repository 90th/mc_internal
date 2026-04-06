#include "mc_internal/hook/render_hook.hpp"

#include <atomic>
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
  std::atomic_thread_fence(std::memory_order_acquire);

  if (!g_ctx) {
    if (g_original_swap_buffers) g_original_swap_buffers(window);
    return;
  }

  // Suppress GLFW error callback during our GLFW calls to prevent
  // transient GLFW_NOT_INITIALIZED errors on early frames.
  GLFWerrorfun prev_error_callback = nullptr;
  if (g_ctx->glfw.set_error_callback) {
    prev_error_callback = g_ctx->glfw.set_error_callback(nullptr);
  }

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

  // attach the render thread to the JVM once and keep it attached for the
  // lifetime of the hook. this avoids per-frame attach/detach overhead.
  if (!g_ctx->jvm_attachment) {
    auto attachment = AttachCurrentThread(g_ctx->jvm);
    if (attachment) { g_ctx->jvm_attachment.emplace(std::move(*attachment)); }
  }

  ImGuiIO& io = ImGui::GetIO();
  io.DisplaySize = ImVec2(static_cast<float>(window_w), static_cast<float>(window_h));
  if (window_w > 0 && window_h > 0) {
    io.DisplayFramebufferScale =
        ImVec2(static_cast<float>(display_w) / static_cast<float>(window_w),
               static_cast<float>(display_h) / static_cast<float>(window_h));
  }
  io.DeltaTime = 1.0f / 60.0f;
  if (g_ctx->glfw.get_time) {
    static double last_time = 0.0;
    double current_time = g_ctx->glfw.get_time();
    io.DeltaTime = last_time > 0 ? (float)(current_time - last_time) : 1.0f / 60.0f;
    last_time = current_time;
  }

  // --- save GL state BEFORE NewFrame (it can recreate device objects) ---
  GLint last_fbo = 0;
  glGetIntegerv(GL_FRAMEBUFFER_BINDING, &last_fbo);

  GLboolean last_enable_primitive_restart = glIsEnabled(GL_PRIMITIVE_RESTART);
  GLint last_active_texture = 0;
  glGetIntegerv(GL_ACTIVE_TEXTURE, &last_active_texture);
  GLint last_texture = 0;
  glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
  GLint last_sampler = 0;
  glGetIntegerv(GL_SAMPLER_BINDING, &last_sampler);

  // neutralize all unpack state the game may leave dirty —
  // imgui only resets ROW_LENGTH and ALIGNMENT, but leftover
  // SKIP_PIXELS / SKIP_ROWS / SWAP_BYTES from lwjgl will
  // corrupt the font atlas texture.
  GLint last_pbo = 0;
  glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING, &last_pbo);

  GLint last_unpack_row_length = 0;
  glGetIntegerv(GL_UNPACK_ROW_LENGTH, &last_unpack_row_length);
  GLint last_unpack_alignment = 4;
  glGetIntegerv(GL_UNPACK_ALIGNMENT, &last_unpack_alignment);
  GLint last_skip_pixels = 0;
  GLint last_skip_rows = 0;
  GLboolean last_swap_bytes = GL_FALSE;
  glGetIntegerv(GL_UNPACK_SKIP_PIXELS, &last_skip_pixels);
  glGetIntegerv(GL_UNPACK_SKIP_ROWS, &last_skip_rows);
  glGetBooleanv(GL_UNPACK_SWAP_BYTES, &last_swap_bytes);

  // --- set clean GL state for ImGui ---
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glDisable(GL_PRIMITIVE_RESTART);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, 0);
  glBindSampler(0, 0);
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
  glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
  glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
  glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
  glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_FALSE);

  ImGui_ImplOpenGL3_NewFrame();
  ImGui::NewFrame();

  ProcessInput(window, *g_ctx);
  g_ctx->module_manager.Render3d(*g_ctx);
  RenderMenu(window, *g_ctx);

  ImGui::Render();

  if (ImDrawData* draw_data = ImGui::GetDrawData()) { ImGui_ImplOpenGL3_RenderDrawData(draw_data); }

  // --- restore GL state ---
  if (last_enable_primitive_restart) { glEnable(GL_PRIMITIVE_RESTART); }
  glActiveTexture(last_active_texture);
  glBindTexture(GL_TEXTURE_2D, last_texture);
  glBindSampler(0, last_sampler);
  glPixelStorei(GL_UNPACK_ROW_LENGTH, last_unpack_row_length);
  glPixelStorei(GL_UNPACK_ALIGNMENT, last_unpack_alignment);
  glPixelStorei(GL_UNPACK_SKIP_PIXELS, last_skip_pixels);
  glPixelStorei(GL_UNPACK_SKIP_ROWS, last_skip_rows);
  glPixelStorei(GL_UNPACK_SWAP_BYTES, last_swap_bytes);
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, static_cast<GLuint>(last_pbo));
  glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(last_fbo));

  if (g_ctx->glfw.set_error_callback && prev_error_callback) {
    g_ctx->glfw.set_error_callback(prev_error_callback);
  }

  if (g_original_swap_buffers != nullptr) { g_original_swap_buffers(window); }
}

}  // namespace

std::expected<void, BootstrapError> InstallRenderHook(JavaVM* jvm) {
  auto lookup = LocateGlfwInLwjgl();
  if (lookup.swap_buffers_address == nullptr) {
    return std::unexpected(BootstrapError::kSwapBuffersLookupFailed);
  }

  // populate the context before the hook goes live so the render thread
  // doesn't race against uninitialized state.
  g_ctx = std::make_unique<OverlayContext>();
  g_ctx->jvm = jvm;
  g_ctx->glfw = std::move(lookup.functions);

  // ensure all writes to g_ctx are visible to the render thread before the
  // hook goes live — the render thread reads g_ctx after an acquire fence.
  std::atomic_thread_fence(std::memory_order_release);

  static_assert(sizeof(GlfwSwapBuffersFn) == sizeof(lm_address_t));

  const lm_size_t swap_hook_size =
      LM_HookCode(reinterpret_cast<lm_address_t>(lookup.swap_buffers_address),
                  reinterpret_cast<lm_address_t>(&HkGlfwSwapBuffers),
                  reinterpret_cast<lm_address_t*>(&g_original_swap_buffers));

  if (swap_hook_size == 0 || g_original_swap_buffers == nullptr) {
    g_ctx.reset();
    return std::unexpected(BootstrapError::kSwapBuffersHookFailed);
  }

  return {};
}

}  // namespace mc_internal
