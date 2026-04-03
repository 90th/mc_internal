#include "mc_internal/sdk/jni_cache.hpp"

#include <print>

#include "mc_internal/sdk/mappings.hpp"
#include "mc_internal/utils/logging.hpp"

namespace mc_internal {

namespace {

jclass CacheGlobalClass(const JniEnv& env, std::string_view class_name) {
  auto local_class = env.FindClass(class_name);
  if (!local_class) { return nullptr; }

  jobject global_class = env->NewGlobalRef(local_class.get());
  if (global_class != nullptr && !env->ExceptionCheck()) {
    return static_cast<jclass>(global_class);
  }

  if (global_class != nullptr) { env->DeleteGlobalRef(global_class); }

  (void)env.ClearException("new global ref");
  std::println("{} jni new global ref failed: {}", kLogPrefix, class_name);
  std::fflush(stdout);
  return nullptr;
}

}  // namespace

bool JniCache::Initialize(const JniEnv& env) noexcept {
  if (initialized_) { return true; }

  if (!env) {
    PrintStatus("jni cache initialization failed: missing jni env");
    return false;
  }

  minecraft_client_class = CacheGlobalClass(env, kMinecraftClientClass);
  client_player_entity_class = CacheGlobalClass(env, kClientPlayerEntityClass);
  client_world_class = CacheGlobalClass(env, kClientWorldClass);
  entity_class = CacheGlobalClass(env, kEntityClass);
  java_lang_iterable_class = CacheGlobalClass(env, kJavaLangIterableClass);
  java_util_iterator_class = CacheGlobalClass(env, kJavaUtilIteratorClass);

  if (minecraft_client_class == nullptr || client_player_entity_class == nullptr ||
      client_world_class == nullptr || entity_class == nullptr ||
      java_lang_iterable_class == nullptr || java_util_iterator_class == nullptr) {
    Reset(env.get());
    PrintStatus("jni cache initialization failed: missing class handles");
    return false;
  }

  minecraft_client_get_instance = env.GetStaticMethodID(minecraft_client_class,
                                                        kMinecraftClientGetInstanceMethod,
                                                        kMinecraftClientGetInstanceSignature,
                                                        kMinecraftClientClass);
  minecraft_client_player_field = env.GetFieldID(minecraft_client_class,
                                                 kMinecraftClientPlayerField,
                                                 kMinecraftClientPlayerFieldSignature,
                                                 kMinecraftClientClass);
  minecraft_client_world_field = env.GetFieldID(minecraft_client_class,
                                                kMinecraftClientWorldField,
                                                kMinecraftClientWorldFieldSignature,
                                                kMinecraftClientClass);
  iterable_iterator = env.GetMethodID(java_lang_iterable_class,
                                      kIterableIteratorMethod,
                                      kIterableIteratorSignature,
                                      kJavaLangIterableClass);
  iterator_has_next = env.GetMethodID(java_util_iterator_class,
                                      kIteratorHasNextMethod,
                                      kIteratorHasNextSignature,
                                      kJavaUtilIteratorClass);
  iterator_next = env.GetMethodID(java_util_iterator_class,
                                  kIteratorNextMethod,
                                  kIteratorNextSignature,
                                  kJavaUtilIteratorClass);

  entity_get_x =
      env.GetMethodID(entity_class, kEntityGetXMethod, kEntityGetCoordSignature, kEntityClass);
  entity_get_y =
      env.GetMethodID(entity_class, kEntityGetYMethod, kEntityGetCoordSignature, kEntityClass);
  entity_get_z =
      env.GetMethodID(entity_class, kEntityGetZMethod, kEntityGetCoordSignature, kEntityClass);

  if (minecraft_client_get_instance == nullptr || minecraft_client_player_field == nullptr ||
      minecraft_client_world_field == nullptr || entity_get_x == nullptr ||
      entity_get_y == nullptr || entity_get_z == nullptr || iterable_iterator == nullptr ||
      iterator_has_next == nullptr || iterator_next == nullptr) {
    Reset(env.get());
    PrintStatus("jni cache initialization failed: missing member handles");
    return false;
  }

  initialized_ = true;
  PrintStatus("jni cache initialized");
  return true;
}

void JniCache::Reset(JNIEnv* env) noexcept {
  if (env != nullptr && minecraft_client_class != nullptr) {
    env->DeleteGlobalRef(minecraft_client_class);
  }
  if (env != nullptr && client_player_entity_class != nullptr) {
    env->DeleteGlobalRef(client_player_entity_class);
  }
  if (env != nullptr && client_world_class != nullptr) { env->DeleteGlobalRef(client_world_class); }
  if (env != nullptr && entity_class != nullptr) { env->DeleteGlobalRef(entity_class); }
  if (env != nullptr && java_lang_iterable_class != nullptr) {
    env->DeleteGlobalRef(java_lang_iterable_class);
  }
  if (env != nullptr && java_util_iterator_class != nullptr) {
    env->DeleteGlobalRef(java_util_iterator_class);
  }

  minecraft_client_class = nullptr;
  client_player_entity_class = nullptr;
  client_world_class = nullptr;
  entity_class = nullptr;
  java_lang_iterable_class = nullptr;
  java_util_iterator_class = nullptr;

  minecraft_client_get_instance = nullptr;
  minecraft_client_player_field = nullptr;
  minecraft_client_world_field = nullptr;
  client_world_get_entities = nullptr;
  entity_get_x = nullptr;
  entity_get_y = nullptr;
  entity_get_z = nullptr;
  iterable_iterator = nullptr;
  iterator_has_next = nullptr;
  iterator_next = nullptr;

  initialized_ = false;
}

}  // namespace mc_internal
