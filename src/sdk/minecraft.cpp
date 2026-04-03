#include "mc_internal/sdk/minecraft.hpp"

#include <print>

#include "mc_internal/utils/logging.hpp"

namespace mc_internal {

namespace {

double CallEntityCoordinateMethod(const JniEnv& env,
                                  jobject entity,
                                  jmethodID method_id,
                                  std::string_view method_name) {
  if (!env || entity == nullptr || method_id == nullptr) { return 0.0; }

  const jdouble value = env->CallDoubleMethod(entity, method_id);
  if (!env->ExceptionCheck()) { return static_cast<double>(value); }

  (void)env.ClearException("call double method");
  std::println("{} jni call double method failed: {}.{} {}",
               kLogPrefix,
               kEntityClass,
               method_name,
               kEntityGetCoordSignature);
  std::fflush(stdout);
  return 0.0;
}

}  // namespace

ScopedLocalRef<jobject> Minecraft::GetInstance(const JniEnv& env, const JniCache& cache) {
  if (!env || !cache.is_initialized() || cache.minecraft_client_class == nullptr ||
      cache.minecraft_client_get_instance == nullptr) {
    return {};
  }

  return env.CallStaticObjectMethod(cache.minecraft_client_class,
                                    cache.minecraft_client_get_instance,
                                    kMinecraftClientClass,
                                    kMinecraftClientGetInstanceMethod,
                                    kMinecraftClientGetInstanceSignature);
}

ScopedLocalRef<jobject>
Minecraft::GetLocalPlayer(const JniEnv& env, const JniCache& cache, jobject minecraft_instance) {
  if (!env || !cache.is_initialized() || minecraft_instance == nullptr ||
      cache.minecraft_client_player_field == nullptr) {
    return {};
  }

  if (jobject player = env->GetObjectField(minecraft_instance, cache.minecraft_client_player_field);
      player != nullptr) {
    if (!env->ExceptionCheck()) { return env.TakeLocal(player); }

    env->DeleteLocalRef(player);
  }

  if (!env->ExceptionCheck()) { return {}; }

  (void)env.ClearException("get object field");
  std::println("{} jni get object field failed: {}.{} {}",
               kLogPrefix,
               kMinecraftClientClass,
               kMinecraftClientPlayerField,
               kMinecraftClientPlayerFieldSignature);
  std::fflush(stdout);
  return {};
}

std::tuple<double, double, double>
ClientPlayerEntity::GetCoordinates(const JniEnv& env, const JniCache& cache, jobject player) {
  if (!env || !cache.is_initialized() || player == nullptr || cache.entity_get_x == nullptr ||
      cache.entity_get_y == nullptr || cache.entity_get_z == nullptr) {
    return {0.0, 0.0, 0.0};
  }

  return {CallEntityCoordinateMethod(env, player, cache.entity_get_x, kEntityGetXMethod),
          CallEntityCoordinateMethod(env, player, cache.entity_get_y, kEntityGetYMethod),
          CallEntityCoordinateMethod(env, player, cache.entity_get_z, kEntityGetZMethod)};
}

ScopedLocalRef<jclass> ClientWorld::FindClass(const JniEnv& env, const JniCache& cache) {
  if (!env || !cache.is_initialized() || cache.client_world_class == nullptr) { return {}; }

  if (jclass local_class = static_cast<jclass>(env->NewLocalRef(cache.client_world_class));
      local_class != nullptr) {
    if (!env->ExceptionCheck()) { return env.TakeLocal(local_class); }

    env->DeleteLocalRef(local_class);
  }

  (void)env.ClearException("new local ref");
  std::println("{} jni new local ref failed: {}", kLogPrefix, kClientWorldClass);
  std::fflush(stdout);
  return {};
}

jobject ClientWorld::GetEntities(const JniEnv& env, const JniCache& cache, jobject world_instance) {
  static_cast<void>(env);
  static_cast<void>(cache);
  static_cast<void>(world_instance);
  return nullptr;
}

}  // namespace mc_internal
