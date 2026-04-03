#include "mc_internal/hook/glfw_bindings.hpp"

#include <dlfcn.h>
#include <link.h>
#include <string_view>

namespace mc_internal {

namespace {

struct LookupCallbackData {
  void* swap_buffers_address = nullptr;
  GlfwFunctions functions{};
};

int LocateLwjglGlfwCallback(struct dl_phdr_info* info, size_t /*size*/, void* data) {
  const char* module_path = info->dlpi_name != nullptr ? info->dlpi_name : "";
  const std::string_view path(module_path);

  if (path.find("glfw") == std::string_view::npos || path.find("lwjgl") == std::string_view::npos) {
    return 0;
  }

  void* handle = dlopen(info->dlpi_name, RTLD_NOLOAD | RTLD_LAZY);
  if (handle == nullptr) { return 1; }

  auto* result = static_cast<LookupCallbackData*>(data);
  result->swap_buffers_address = dlsym(handle, "glfwSwapBuffers");
  result->functions.get_framebuffer_size =
      reinterpret_cast<GlfwGetFramebufferSizeFn>(dlsym(handle, "glfwGetFramebufferSize"));
  result->functions.get_window_size =
      reinterpret_cast<GlfwGetWindowSizeFn>(dlsym(handle, "glfwGetWindowSize"));
  result->functions.get_key = reinterpret_cast<GlfwGetKeyFn>(dlsym(handle, "glfwGetKey"));
  result->functions.get_mouse_button =
      reinterpret_cast<GlfwGetMouseButtonFn>(dlsym(handle, "glfwGetMouseButton"));
  result->functions.get_cursor_pos =
      reinterpret_cast<GlfwGetCursorPosFn>(dlsym(handle, "glfwGetCursorPos"));
  result->functions.set_input_mode =
      reinterpret_cast<GlfwSetInputModeFn>(dlsym(handle, "glfwSetInputMode"));
  result->functions.get_input_mode =
      reinterpret_cast<GlfwGetInputModeFn>(dlsym(handle, "glfwGetInputMode"));
  result->functions.get_current_context =
      reinterpret_cast<GlfwGetCurrentContextFn>(dlsym(handle, "glfwGetCurrentContext"));

  return 1;
}

}  // namespace

GlfwLookupResult LocateGlfwInLwjgl() {
  LookupCallbackData data{};
  dl_iterate_phdr(LocateLwjglGlfwCallback, &data);
  return GlfwLookupResult{data.swap_buffers_address, data.functions};
}

}  // namespace mc_internal
