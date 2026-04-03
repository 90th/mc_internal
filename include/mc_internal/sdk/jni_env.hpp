#pragma once

#include <jni.h>

#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include "mc_internal/utils/logging.hpp"

namespace mc_internal {

template <typename T>
class ScopedLocalRef {
  static_assert(std::is_pointer_v<T>);
  static_assert(std::is_convertible_v<T, jobject>);

 public:
  ScopedLocalRef() = default;

  ScopedLocalRef(JNIEnv* env, T reference) : env_(env), reference_(reference) {}

  ScopedLocalRef(const ScopedLocalRef&) = delete;
  ScopedLocalRef& operator=(const ScopedLocalRef&) = delete;

  ScopedLocalRef(ScopedLocalRef&& other) noexcept
      : env_(std::exchange(other.env_, nullptr)),
        reference_(std::exchange(other.reference_, nullptr)) {}

  ScopedLocalRef& operator=(ScopedLocalRef&& other) noexcept {
    if (this == &other) { return *this; }

    Reset();
    env_ = std::exchange(other.env_, nullptr);
    reference_ = std::exchange(other.reference_, nullptr);
    return *this;
  }

  ~ScopedLocalRef() { Reset(); }

  [[nodiscard]] T get() const noexcept { return reference_; }
  [[nodiscard]] explicit operator bool() const noexcept { return reference_ != nullptr; }

  [[nodiscard]] T Release() noexcept { return std::exchange(reference_, nullptr); }

  void Reset(T reference = nullptr) noexcept {
    if (env_ != nullptr && reference_ != nullptr) { env_->DeleteLocalRef(reference_); }
    reference_ = reference;
  }

 private:
  JNIEnv* env_ = nullptr;
  T reference_ = nullptr;
};

// Thin, thread-local wrapper around a JNI environment. Construct this from the
// current thread's JNIEnv* whenever JNI work is needed; do not cache it across
// threads.
struct JniEnv {
  explicit JniEnv(JNIEnv* env) : env_(env) {}

  [[nodiscard]] JNIEnv* get() const noexcept { return env_; }
  [[nodiscard]] JNIEnv* operator->() const noexcept { return env_; }
  [[nodiscard]] explicit operator bool() const noexcept { return env_ != nullptr; }

  template <typename T>
  [[nodiscard]] ScopedLocalRef<T> TakeLocal(T reference) const noexcept {
    return ScopedLocalRef<T>(env_, reference);
  }

  [[nodiscard]] ScopedLocalRef<jclass> FindClass(std::string_view class_name) const noexcept {
    if (env_ == nullptr) {
      LogClassFailure("find class", class_name);
      return {};
    }

    const std::string class_name_buffer(class_name);
    if (jclass found_class = env_->FindClass(class_name_buffer.c_str()); found_class != nullptr) {
      return TakeLocal(found_class);
    }

    (void)ClearException("find class");
    LogClassFailure("find class", class_name);
    return {};
  }

  [[nodiscard]] jmethodID GetMethodID(jclass clazz,
                                      std::string_view method_name,
                                      std::string_view signature,
                                      std::string_view owner = {}) const noexcept {
    return GetMethodIdImpl(clazz, method_name, signature, owner, false);
  }

  [[nodiscard]] jmethodID GetStaticMethodID(jclass clazz,
                                            std::string_view method_name,
                                            std::string_view signature,
                                            std::string_view owner = {}) const noexcept {
    return GetMethodIdImpl(clazz, method_name, signature, owner, true);
  }

  [[nodiscard]] jfieldID GetFieldID(jclass clazz,
                                    std::string_view field_name,
                                    std::string_view signature,
                                    std::string_view owner = {}) const noexcept {
    if (env_ == nullptr || clazz == nullptr) {
      LogMemberFailure("get field id", owner, field_name, signature);
      return nullptr;
    }

    const std::string field_name_buffer(field_name);
    const std::string signature_buffer(signature);
    if (jfieldID field_id =
            env_->GetFieldID(clazz, field_name_buffer.c_str(), signature_buffer.c_str());
        field_id != nullptr) {
      return field_id;
    }

    (void)ClearException("get field id");
    LogMemberFailure("get field id", owner, field_name, signature);
    return nullptr;
  }

  [[nodiscard]] ScopedLocalRef<jobject>
  CallStaticObjectMethod(jclass clazz,
                         jmethodID method_id,
                         std::string_view owner,
                         std::string_view method_name,
                         std::string_view signature) const noexcept {
    if (env_ == nullptr || clazz == nullptr || method_id == nullptr) {
      LogMemberFailure("call static object method", owner, method_name, signature);
      return {};
    }

    if (jobject instance = env_->CallStaticObjectMethod(clazz, method_id); instance != nullptr) {
      if (!env_->ExceptionCheck()) { return TakeLocal(instance); }

      env_->DeleteLocalRef(instance);
    }

    (void)ClearException("call static object method");
    LogMemberFailure("call static object method", owner, method_name, signature);
    return {};
  }

  [[nodiscard]] bool ClearException(std::string_view context) const noexcept {
    if (env_ == nullptr || !env_->ExceptionCheck()) { return false; }

    env_->ExceptionClear();
    std::println("{} cleared pending jni exception after {}", kLogPrefix, context);
    std::fflush(stdout);
    return true;
  }

 private:
  [[nodiscard]] jmethodID GetMethodIdImpl(jclass clazz,
                                          std::string_view method_name,
                                          std::string_view signature,
                                          std::string_view owner,
                                          bool is_static) const noexcept {
    if (env_ == nullptr || clazz == nullptr) {
      LogMemberFailure(
          is_static ? "get static method id" : "get method id", owner, method_name, signature);
      return nullptr;
    }

    const std::string method_name_buffer(method_name);
    const std::string signature_buffer(signature);
    const jmethodID method_id =
        is_static
            ? env_->GetStaticMethodID(clazz, method_name_buffer.c_str(), signature_buffer.c_str())
            : env_->GetMethodID(clazz, method_name_buffer.c_str(), signature_buffer.c_str());
    if (method_id != nullptr) { return method_id; }

    (void)ClearException(is_static ? "get static method id" : "get method id");
    LogMemberFailure(
        is_static ? "get static method id" : "get method id", owner, method_name, signature);
    return nullptr;
  }

  void LogClassFailure(std::string_view action, std::string_view class_name) const noexcept {
    std::println("{} jni {} failed: {}", kLogPrefix, action, class_name);
    std::fflush(stdout);
  }

  void LogMemberFailure(std::string_view action,
                        std::string_view owner,
                        std::string_view member_name,
                        std::string_view signature) const noexcept {
    const std::string_view resolved_owner = owner.empty() ? "<unknown class>" : owner;
    std::println(
        "{} jni {} failed: {}.{} {}", kLogPrefix, action, resolved_owner, member_name, signature);
    std::fflush(stdout);
  }

  JNIEnv* env_ = nullptr;
};

}  // namespace mc_internal
