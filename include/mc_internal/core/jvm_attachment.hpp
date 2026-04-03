#pragma once

#include <expected>
#include <jni.h>
#include <utility>

#include "mc_internal/utils/errors.hpp"

namespace mc_internal {

// RAII wrapper around a JNI thread attachment. Calls DetachCurrentThread on
// destruction only when this object performed the attachment (not when the
// thread was already attached on entry).
class JvmThreadAttachment {
 public:
  JvmThreadAttachment(JavaVM* jvm, JNIEnv* env, bool should_detach)
      : jvm_(jvm), env_(env), should_detach_(should_detach) {}

  JvmThreadAttachment(const JvmThreadAttachment&) = delete;
  JvmThreadAttachment& operator=(const JvmThreadAttachment&) = delete;

  JvmThreadAttachment(JvmThreadAttachment&& other) noexcept
      : jvm_(std::exchange(other.jvm_, nullptr)),
        env_(std::exchange(other.env_, nullptr)),
        should_detach_(std::exchange(other.should_detach_, false)) {}

  JvmThreadAttachment& operator=(JvmThreadAttachment&& other) noexcept {
    if (this == &other) { return *this; }
    if (should_detach_ && jvm_ != nullptr) { jvm_->DetachCurrentThread(); }

    jvm_ = std::exchange(other.jvm_, nullptr);
    env_ = std::exchange(other.env_, nullptr);
    should_detach_ = std::exchange(other.should_detach_, false);
    return *this;
  }

  ~JvmThreadAttachment() {
    if (should_detach_ && jvm_ != nullptr) { jvm_->DetachCurrentThread(); }
  }

  [[nodiscard]] JavaVM* jvm() const noexcept { return jvm_; }
  [[nodiscard]] JNIEnv* env() const noexcept { return env_; }

 private:
  JavaVM* jvm_ = nullptr;
  JNIEnv* env_ = nullptr;
  bool should_detach_ = false;
};

// Attaches the calling thread to the JVM if not already attached.
// Returns a JvmThreadAttachment that will detach on destruction only when
// this call performed the attachment.
[[nodiscard]] std::expected<JvmThreadAttachment, BootstrapError> AttachCurrentThread(JavaVM* jvm);

}  // namespace mc_internal
