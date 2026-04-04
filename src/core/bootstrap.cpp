#include "mc_internal/core/bootstrap.hpp"

#include <chrono>
#include <exception>
#include <print>
#include <thread>

#include "mc_internal/core/jni_hook.hpp"
#include "mc_internal/core/jvm_attachment.hpp"
#include "mc_internal/hook/render_hook.hpp"
#include "mc_internal/utils/logging.hpp"

namespace mc_internal {

static void BootstrapImpl(JavaVM* jvm) {
  PrintStatus("bootstrap thread started via jvmti");

  auto attachment = AttachCurrentThread(jvm);
  if (!attachment) {
    PrintFailure("thread attach", attachment.error());
    return;
  }

  std::println("{} bootstrap env {}", kLogPrefix, static_cast<const void*>(attachment->env()));

  if (const auto init = InitializeJniHook(attachment->jvm()); !init) {
    PrintFailure("jnihook init", init.error());
    return;
  }

  std::println("{} jnihook ready", kLogPrefix);

  for (int attempt = 1; attempt <= 60; ++attempt) {
    if (const auto render_hook = InstallRenderHook(attachment->jvm()); render_hook) {
      PrintStatus("render hook installed");
      return;
    } else if (render_hook.error() != BootstrapError::kSwapBuffersLookupFailed) {
      PrintFailure("render hook", render_hook.error());
      return;
    }

    if (attempt < 60) {
      std::println("{} render hook waiting for glfw (attempt {}/60)", kLogPrefix, attempt);
      std::fflush(stdout);
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  }

  PrintFailure("render hook", BootstrapError::kSwapBuffersLookupFailed);
}

void Bootstrap(JavaVM* jvm) {
  try {
    BootstrapImpl(jvm);
  } catch (...) { PrintStatus("bootstrap crashed"); }
}

}  // namespace mc_internal
