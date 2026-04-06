#pragma once

#include <jni.h>
#include <optional>

#include "mc_internal/core/jvm_attachment.hpp"
#include "mc_internal/features/module_manager.hpp"
#include "mc_internal/hook/glfw_bindings.hpp"
#include "mc_internal/sdk/jni_cache.hpp"

namespace mc_internal {

// Centralized per-frame state for the overlay. Instantiated once by
// InstallRenderHook and held as a static unique_ptr in render_hook.cpp.
// All overlay subsystems receive a reference to this struct rather than
// maintaining their own module-level statics.
struct OverlayContext {
  JavaVM* jvm = nullptr;
  bool show_menu = false;
  GLFWwindow* pinned_window = nullptr;
  int window_width = 0;
  int window_height = 0;
  int display_width = 0;
  int display_height = 0;

  // ImGui lifetime flags managed by imgui_manager.
  bool imgui_initialized = false;
  bool imgui_init_failed = false;

  // Input state preserved across frames.
  bool insert_was_down = false;
  int original_cursor_mode = GLFW_CURSOR_NORMAL;

  // Dynamically resolved GLFW entry points.
  GlfwFunctions glfw{};

  // Persistent JVM attachment for the render thread. Created once on first
  // use and kept alive for the lifetime of the hook to avoid per-frame
  // attach/detach overhead and thread corruption.
  std::optional<JvmThreadAttachment> jvm_attachment;

  // Per-process JNI cache and feature registry used by the render thread.
  mutable JniCache jni_cache{};
  mutable ModuleManager module_manager{};
};

}  // namespace mc_internal
