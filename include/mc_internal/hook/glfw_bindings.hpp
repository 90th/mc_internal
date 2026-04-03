#pragma once

#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES
#endif
#include <GL/gl.h>
#include <GL/glext.h>
#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif
#include <GLFW/glfw3.h>

namespace mc_internal {

using GlfwSwapBuffersFn = void (*)(GLFWwindow*);
using GlfwGetFramebufferSizeFn = void (*)(GLFWwindow*, int*, int*);
using GlfwGetWindowSizeFn = void (*)(GLFWwindow*, int*, int*);
using GlfwGetKeyFn = int (*)(GLFWwindow*, int);
using GlfwGetMouseButtonFn = int (*)(GLFWwindow*, int);
using GlfwGetCursorPosFn = void (*)(GLFWwindow*, double*, double*);
using GlfwSetInputModeFn = void (*)(GLFWwindow*, int, int);
using GlfwGetInputModeFn = int (*)(GLFWwindow*, int);
using GlfwGetCurrentContextFn = GLFWwindow* (*)();

// All dynamically resolved GLFW entry points needed by the overlay.
struct GlfwFunctions {
  GlfwGetFramebufferSizeFn get_framebuffer_size = nullptr;
  GlfwGetWindowSizeFn get_window_size = nullptr;
  GlfwGetKeyFn get_key = nullptr;
  GlfwGetMouseButtonFn get_mouse_button = nullptr;
  GlfwGetCursorPosFn get_cursor_pos = nullptr;
  GlfwSetInputModeFn set_input_mode = nullptr;
  GlfwGetInputModeFn get_input_mode = nullptr;
  GlfwGetCurrentContextFn get_current_context = nullptr;
};

// Bundles the result of a successful LWJGL GLFW module scan.
struct GlfwLookupResult {
  void* swap_buffers_address = nullptr;
  GlfwFunctions functions{};
};

// Walks loaded shared libraries searching for the LWJGL GLFW module and
// resolves all function pointers needed by the overlay.
[[nodiscard]] GlfwLookupResult LocateGlfwInLwjgl();

}  // namespace mc_internal
