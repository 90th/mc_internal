#include "mc_internal/hook/glfw_bindings.hpp"

#include <dlfcn.h>
#include <link.h>
#include <string>
#include <string_view>

#include "mc_internal/utils/logging.hpp"

namespace mc_internal {

namespace {

struct LookupCallbackData {
  void* swap_buffers_address = nullptr;
  GlfwFunctions functions{};
  int best_score = -1;
  std::string selected_path;
};

[[nodiscard]] bool IsGlfwLibraryPath(std::string_view path) {
  return path.contains("libglfw.so") || path.contains("libglfw_wayland.so");
}

[[nodiscard]] bool IsSystemGlfwPath(std::string_view path) {
  return path.starts_with("/usr/lib") || path.starts_with("/lib") ||
         path.starts_with("/usr/local/lib") || path.starts_with("/app/lib");
}

[[nodiscard]] int ScoreGlfwLibraryPath(std::string_view path) {
  if (path.contains("liblwjgl_glfw")) { return -1; }
  if (!IsGlfwLibraryPath(path)) { return -1; }
  if (IsSystemGlfwPath(path)) { return -1; }

  int score = 0;
  if (path.contains("natives")) { score += 100; }
  if (path.contains("PrismLauncher")) { score += 50; }
  if (path.contains("instances")) { score += 25; }
  if (!path.empty() && path[0] == '/') { score += 10; }
  return score;
}

[[nodiscard]] bool HasRequiredFunctions(const GlfwFunctions& functions) {
  return functions.get_framebuffer_size != nullptr && functions.get_window_size != nullptr &&
         functions.get_key != nullptr && functions.get_mouse_button != nullptr &&
         functions.get_cursor_pos != nullptr && functions.set_input_mode != nullptr &&
         functions.get_input_mode != nullptr && functions.get_current_context != nullptr;
}

int LocateLwjglGlfwCallback(struct dl_phdr_info* info, size_t, void* data) {
  const std::string_view path(info->dlpi_name != nullptr ? info->dlpi_name : "");

  const int score = ScoreGlfwLibraryPath(path);
  if (score < 0) { return 0; }

  void* handle = dlopen(info->dlpi_name, RTLD_NOLOAD | RTLD_LAZY);
  if (!handle) { return 0; }

  // THE FIX: We force a GLFW function that requires initialization.
  // If the library is uninitialized (or a Wayland/X11 ghost), it catches the
  // error internally and we skip it, waiting for Minecraft to boot the real one.
  using GlfwGetCurrentContextFn = GLFWwindow* (*)();
  using GlfwGetErrorFn = int (*)(const char**);
  auto get_current_context = (GlfwGetCurrentContextFn)dlsym(handle, "glfwGetCurrentContext");
  auto get_error = (GlfwGetErrorFn)dlsym(handle, "glfwGetError");

  if (get_current_context != nullptr && get_error != nullptr) {
    get_current_context();  // Triggers GLFW_NOT_INITIALIZED if the library is not active.

    const char* desc = nullptr;
    if (get_error(&desc) == 0x00010001) {  // 0x00010001 is GLFW_NOT_INITIALIZED
      return 0;                            // Skip this uninitialized ghost library cunt!
    }
  }

  GlfwFunctions functions{};
  functions.get_framebuffer_size =
      (GlfwGetFramebufferSizeFn)dlsym(handle, "glfwGetFramebufferSize");
  functions.get_window_size = (GlfwGetWindowSizeFn)dlsym(handle, "glfwGetWindowSize");
  functions.get_key = (GlfwGetKeyFn)dlsym(handle, "glfwGetKey");
  functions.get_mouse_button = (GlfwGetMouseButtonFn)dlsym(handle, "glfwGetMouseButton");
  functions.get_cursor_pos = (GlfwGetCursorPosFn)dlsym(handle, "glfwGetCursorPos");
  functions.set_input_mode = (GlfwSetInputModeFn)dlsym(handle, "glfwSetInputMode");
  functions.get_input_mode = (GlfwGetInputModeFn)dlsym(handle, "glfwGetInputMode");
  functions.get_current_context = (GlfwGetCurrentContextFn)dlsym(handle, "glfwGetCurrentContext");

  void* swap_addr = dlsym(handle, "glfwSwapBuffers");
  if (!swap_addr || !HasRequiredFunctions(functions)) { return 0; }

  auto* result = static_cast<LookupCallbackData*>(data);
  if (score <= result->best_score) { return 0; }

  result->best_score = score;
  result->swap_buffers_address = swap_addr;
  result->functions = functions;
  result->selected_path = std::string(path);
  return 0;
}
}  // namespace

GlfwLookupResult LocateGlfwInLwjgl() {
  static std::string last_logged_selected_path;

  LookupCallbackData data{};
  dl_iterate_phdr(LocateLwjglGlfwCallback, &data);

  if (!data.selected_path.empty() && data.selected_path != last_logged_selected_path) {
    PrintStatus(std::string("selected glfw candidate: ") + data.selected_path);
    last_logged_selected_path = data.selected_path;
  }

  return {data.swap_buffers_address, data.functions};
}
}  // namespace mc_internal