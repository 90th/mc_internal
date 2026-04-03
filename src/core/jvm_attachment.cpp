#include "mc_internal/core/jvm_attachment.hpp"

namespace mc_internal {

std::expected<JvmThreadAttachment, BootstrapError> AttachCurrentThread(JavaVM* jvm) {
  void* raw_env = nullptr;
  const auto get_env_result = jvm->GetEnv(&raw_env, JNI_VERSION_1_6);

  if (get_env_result == JNI_OK) {
    return JvmThreadAttachment(jvm, static_cast<JNIEnv*>(raw_env), false);
  }
  if (get_env_result != JNI_EDETACHED) {
    return std::unexpected(BootstrapError::kThreadAttachFailed);
  }

  JNIEnv* env = nullptr;
  if (jvm->AttachCurrentThread(reinterpret_cast<void**>(&env), nullptr) != JNI_OK) {
    return std::unexpected(BootstrapError::kThreadAttachFailed);
  }

  return JvmThreadAttachment(jvm, env, true);
}

}  // namespace mc_internal
