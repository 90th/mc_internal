#include "mc_internal/core/jni_hook.hpp"

namespace mc_internal {

std::expected<void, jnihook::result_t> InitializeJniHook(JavaVM* jvm) {
  if (const auto result = jnihook::init(jvm); result != JNIHOOK_OK) {
    return std::unexpected(result);
  }
  return {};
}

}  // namespace mc_internal
