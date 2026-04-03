#pragma once

#include <jni.h>

#include "mc_internal/sdk/jni_env.hpp"

namespace mc_internal {

struct JniCache {
  JniCache() = default;

  JniCache(const JniCache&) = delete;
  JniCache& operator=(const JniCache&) = delete;
  JniCache(JniCache&&) = delete;
  JniCache& operator=(JniCache&&) = delete;

  [[nodiscard]] bool Initialize(const JniEnv& env) noexcept;
  [[nodiscard]] bool is_initialized() const noexcept { return initialized_; }

  jclass minecraft_client_class = nullptr;
  jclass client_player_entity_class = nullptr;
  jclass client_world_class = nullptr;
  jclass entity_class = nullptr;
  jclass java_lang_iterable_class = nullptr;
  jclass java_util_iterator_class = nullptr;

  jmethodID minecraft_client_get_instance = nullptr;
  jfieldID minecraft_client_player_field = nullptr;
  jfieldID minecraft_client_world_field = nullptr;
  jmethodID client_world_get_entities = nullptr;

  jmethodID entity_get_x = nullptr;
  jmethodID entity_get_y = nullptr;
  jmethodID entity_get_z = nullptr;
  jmethodID iterable_iterator = nullptr;
  jmethodID iterator_has_next = nullptr;
  jmethodID iterator_next = nullptr;

 private:
  void Reset(JNIEnv* env) noexcept;

  bool initialized_ = false;
};

}  // namespace mc_internal
