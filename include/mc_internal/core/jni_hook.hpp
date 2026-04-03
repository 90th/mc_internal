#pragma once

#include <expected>
#include <jni.h>
#include <string_view>

#include "jnihook.hpp"

namespace mc_internal {

// Initializes the jnihook library against the given JVM.
[[nodiscard]] std::expected<void, jnihook::result_t> InitializeJniHook(JavaVM* jvm);

// Describe overload for PrintFailure — lives here because jnihook.hpp lacks an include guard
// and must only be pulled in once per translation unit.
constexpr std::string_view Describe(jnihook::result_t error) noexcept {
  switch (error) {
    case JNIHOOK_OK:
      return "ok";
    case JNIHOOK_ERR_GET_JNI:
      return "could not fetch a jni env";
    case JNIHOOK_ERR_GET_JVMTI:
      return "could not fetch a jvmti env";
    case JNIHOOK_ERR_ADD_JVMTI_CAPS:
      return "could not enable the required jvmti caps";
    case JNIHOOK_ERR_SETUP_CLASS_FILE_LOAD_HOOK:
      return "could not arm the class load hook";
    case JNIHOOK_ERR_JNI_OPERATION:
      return "a jni call failed";
    case JNIHOOK_ERR_JVMTI_OPERATION:
      return "a jvmti call failed";
    case JNIHOOK_ERR_CLASS_FILE_CACHE:
      return "class bytes could not be cached";
    case JNIHOOK_ERR_JAVA_EXCEPTION:
      return "a java exception escaped the bootstrap path";
    case JNIHOOK_ERR_CLASS_FILE_FORMAT:
      return "class rewriting produced invalid bytecode";
    case JNIHOOK_ERR_UNKNOWN:
      return "unknown jnihook error";
  }
  return "unrecognized jnihook error";
}

}  // namespace mc_internal
