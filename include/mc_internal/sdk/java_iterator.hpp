#pragma once

#include <cstddef>
#include <iterator>
#include <type_traits>

#include "mc_internal/sdk/jni_cache.hpp"
#include "mc_internal/sdk/jni_env.hpp"

namespace mc_internal {

template <typename T = jobject>
class JavaIterator {
  static_assert(std::is_pointer_v<T>);
  static_assert(std::is_convertible_v<T, jobject>);

 public:
  using iterator_category = std::input_iterator_tag;
  using value_type = ScopedLocalRef<T>;
  using difference_type = std::ptrdiff_t;

  JavaIterator() = default;

  JavaIterator(const JniEnv& env, const JniCache& cache, jobject iterator_object, bool at_end)
      : env_(env),
        cache_(&cache),
        iterator_object_(env.TakeLocal(iterator_object)),
        at_end_(at_end) {
    if (!at_end_) { Advance(); }
  }

  [[nodiscard]] value_type operator*() const { return CloneCurrent(); }

  [[nodiscard]] value_type next() {
    value_type current = CloneCurrent();
    Advance();
    return current;
  }

  JavaIterator& operator++() {
    Advance();
    return *this;
  }

  void operator++(int) { ++(*this); }

  [[nodiscard]] bool operator==(const JavaIterator& other) const noexcept {
    return at_end_ == other.at_end_ &&
           (at_end_ || iterator_object_.get() == other.iterator_object_.get());
  }

  [[nodiscard]] bool operator!=(const JavaIterator& other) const noexcept {
    return !(*this == other);
  }

 private:
  [[nodiscard]] value_type CloneCurrent() const {
    if (!env_ || !current_) { return {}; }

    if (jobject local_ref = env_->NewLocalRef(current_.get()); local_ref != nullptr) {
      if (!env_->ExceptionCheck()) { return env_.TakeLocal(static_cast<T>(local_ref)); }

      env_->DeleteLocalRef(local_ref);
    }

    (void)env_.ClearException("iterator clone local ref");
    return {};
  }

  void Advance() {
    current_.Reset();
    if (!env_ || cache_ == nullptr || !cache_->is_initialized() || !iterator_object_ || at_end_ ||
        cache_->iterator_has_next == nullptr || cache_->iterator_next == nullptr) {
      at_end_ = true;
      return;
    }

    const jboolean has_next =
        env_->CallBooleanMethod(iterator_object_.get(), cache_->iterator_has_next);
    if (env_->ExceptionCheck()) {
      (void)env_.ClearException("iterator hasNext");
      at_end_ = true;
      return;
    }

    if (has_next == JNI_FALSE) {
      at_end_ = true;
      return;
    }

    jobject next = env_->CallObjectMethod(iterator_object_.get(), cache_->iterator_next);
    if (env_->ExceptionCheck()) {
      if (next != nullptr) { env_->DeleteLocalRef(next); }
      (void)env_.ClearException("iterator next");
      at_end_ = true;
      return;
    }

    current_ = env_.TakeLocal(static_cast<T>(next));
    at_end_ = !current_;
  }

  JniEnv env_{nullptr};
  const JniCache* cache_ = nullptr;
  ScopedLocalRef<jobject> iterator_object_{};
  value_type current_{};
  bool at_end_ = true;
};

template <typename T = jobject>
class JavaIterable {
  static_assert(std::is_pointer_v<T>);
  static_assert(std::is_convertible_v<T, jobject>);

 public:
  JavaIterable(const JniEnv& env, const JniCache& cache, jobject iterable_object)
      : env_(env), cache_(&cache), iterable_object_(env.TakeLocal(iterable_object)) {}

  [[nodiscard]] JavaIterator<T> begin() const {
    if (!env_ || cache_ == nullptr || !cache_->is_initialized() || !iterable_object_ ||
        cache_->iterable_iterator == nullptr) {
      return end();
    }

    jobject iterator = env_->CallObjectMethod(iterable_object_.get(), cache_->iterable_iterator);
    if (env_->ExceptionCheck()) {
      if (iterator != nullptr) { env_->DeleteLocalRef(iterator); }
      (void)env_.ClearException("iterable iterator");
      return end();
    }

    return JavaIterator<T>(env_, *cache_, iterator, false);
  }

  [[nodiscard]] JavaIterator<T> end() const { return JavaIterator<T>(); }

 private:
  JniEnv env_{nullptr};
  const JniCache* cache_ = nullptr;
  mutable ScopedLocalRef<jobject> iterable_object_{};
};

}  // namespace mc_internal
