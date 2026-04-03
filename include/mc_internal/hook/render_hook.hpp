#pragma once

#include <expected>
#include <jni.h>

#include "mc_internal/utils/errors.hpp"

namespace mc_internal {

// Locates glfwSwapBuffers in the LWJGL GLFW module, installs a detour via
// libmem, and allocates the OverlayContext. Must be called once from the
// bootstrap thread before the render hook fires.
[[nodiscard]] std::expected<void, BootstrapError> InstallRenderHook(JavaVM* jvm);

}  // namespace mc_internal
