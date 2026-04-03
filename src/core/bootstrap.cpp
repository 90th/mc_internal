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
  std::this_thread::sleep_for(std::chrono::seconds(15));
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

  if (const auto render_hook = InstallRenderHook(); !render_hook) {
    PrintFailure("render hook", render_hook.error());
    return;
  }

  PrintStatus("render hook installed");
}

void Bootstrap(JavaVM* jvm) {
  try {
    BootstrapImpl(jvm);
  } catch (...) { PrintStatus("bootstrap crashed"); }
}

}  // namespace mc_internal
