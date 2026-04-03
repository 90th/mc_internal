#pragma once

#include <jni.h>

namespace mc_internal {

// Launches BootstrapImpl on a detached thread. All errors are logged
// internally; this function never throws.
void Bootstrap(JavaVM* jvm);

}  // namespace mc_internal
