#include "mc_internal/sdk/minecraft.hpp"

#include <print>

#include "mc_internal/utils/logging.hpp"

namespace mc_internal {

namespace {

ScopedLocalRef<jobject> GetObjectFieldReference(const JniEnv& env,
                                                jobject object,
                                                jfieldID field_id,
                                                std::string_view owner,
                                                std::string_view field_name,
                                                std::string_view signature) {
  if (!env || object == nullptr || field_id == nullptr) { return {}; }

  if (jobject reference = env->GetObjectField(object, field_id); reference != nullptr) {
    if (!env->ExceptionCheck()) { return env.TakeLocal(reference); }

    env->DeleteLocalRef(reference);
  }

  if (!env->ExceptionCheck()) { return {}; }

  (void)env.ClearException("get object field");
  std::println(
      "{} jni get object field failed: {}.{} {}", kLogPrefix, owner, field_name, signature);
  std::fflush(stdout);
  return {};
}

ScopedLocalRef<jobject> CallObjectMethodReference(const JniEnv& env,
                                                  jobject object,
                                                  jmethodID method_id,
                                                  std::string_view owner,
                                                  std::string_view method_name,
                                                  std::string_view signature) {
  if (!env || object == nullptr || method_id == nullptr) { return {}; }

  if (jobject reference = env->CallObjectMethod(object, method_id); reference != nullptr) {
    if (!env->ExceptionCheck()) { return env.TakeLocal(reference); }

    env->DeleteLocalRef(reference);
  }

  if (!env->ExceptionCheck()) { return {}; }

  (void)env.ClearException("call object method");
  std::println(
      "{} jni call object method failed: {}.{} {}", kLogPrefix, owner, method_name, signature);
  std::fflush(stdout);
  return {};
}

float CallFloatMethod(const JniEnv& env,
                      jobject object,
                      jmethodID method_id,
                      std::string_view owner,
                      std::string_view method_name,
                      std::string_view signature) {
  if (!env || object == nullptr || method_id == nullptr) { return 0.0f; }

  const jfloat value = env->CallFloatMethod(object, method_id);
  if (!env->ExceptionCheck()) { return static_cast<float>(value); }

  (void)env.ClearException("call float method");
  std::println(
      "{} jni call float method failed: {}.{} {}", kLogPrefix, owner, method_name, signature);
  std::fflush(stdout);
  return 0.0f;
}

bool CallBooleanMethod(const JniEnv& env,
                       jobject object,
                       jmethodID method_id,
                       std::string_view owner,
                       std::string_view method_name,
                       std::string_view signature) {
  if (!env || object == nullptr || method_id == nullptr) { return false; }

  const jboolean value = env->CallBooleanMethod(object, method_id);
  if (!env->ExceptionCheck()) { return value == JNI_TRUE; }

  (void)env.ClearException("call boolean method");
  std::println(
      "{} jni call boolean method failed: {}.{} {}", kLogPrefix, owner, method_name, signature);
  std::fflush(stdout);
  return false;
}

double GetDoubleField(const JniEnv& env,
                      jobject object,
                      jfieldID field_id,
                      std::string_view owner,
                      std::string_view field_name,
                      std::string_view signature) {
  if (!env || object == nullptr || field_id == nullptr) { return 0.0; }

  const jdouble value = env->GetDoubleField(object, field_id);
  if (!env->ExceptionCheck()) { return static_cast<double>(value); }

  (void)env.ClearException("get double field");
  std::println(
      "{} jni get double field failed: {}.{} {}", kLogPrefix, owner, field_name, signature);
  std::fflush(stdout);
  return 0.0;
}

Vec3 ReadVec3d(const JniEnv& env, const JniCache& cache, jobject vec3d_object) {
  if (!env || !cache.is_initialized() || vec3d_object == nullptr ||
      cache.vec3d_x_field == nullptr || cache.vec3d_y_field == nullptr ||
      cache.vec3d_z_field == nullptr) {
    return {};
  }

  return {GetDoubleField(env,
                         vec3d_object,
                         cache.vec3d_x_field,
                         kVec3dClass,
                         kVec3dGetXField,
                         kVec3dCoordSignature),
          GetDoubleField(env,
                         vec3d_object,
                         cache.vec3d_y_field,
                         kVec3dClass,
                         kVec3dGetYField,
                         kVec3dCoordSignature),
          GetDoubleField(env,
                         vec3d_object,
                         cache.vec3d_z_field,
                         kVec3dClass,
                         kVec3dGetZField,
                         kVec3dCoordSignature)};
}

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

  return GetObjectFieldReference(env,
                                 minecraft_instance,
                                 cache.minecraft_client_player_field,
                                 kMinecraftClientClass,
                                 kMinecraftClientPlayerField,
                                 kMinecraftClientPlayerFieldSignature);
}

ScopedLocalRef<jobject>
Minecraft::GetWorld(const JniEnv& env, const JniCache& cache, jobject minecraft_instance) {
  if (!env || !cache.is_initialized() || minecraft_instance == nullptr ||
      cache.minecraft_client_world_field == nullptr) {
    return {};
  }

  return GetObjectFieldReference(env,
                                 minecraft_instance,
                                 cache.minecraft_client_world_field,
                                 kMinecraftClientClass,
                                 kMinecraftClientWorldField,
                                 kMinecraftClientWorldFieldSignature);
}

ScopedLocalRef<jobject>
Minecraft::GetGameRenderer(const JniEnv& env, const JniCache& cache, jobject minecraft_instance) {
  if (!env || !cache.is_initialized() || minecraft_instance == nullptr ||
      cache.minecraft_client_game_renderer_field == nullptr) {
    return {};
  }

  return GetObjectFieldReference(env,
                                 minecraft_instance,
                                 cache.minecraft_client_game_renderer_field,
                                 kMinecraftClientClass,
                                 kMinecraftClientGameRendererField,
                                 kMinecraftClientGameRendererSignature);
}

float Minecraft::GetTickDelta(const JniEnv& env,
                              const JniCache& cache,
                              jobject minecraft_instance) {
  if (!env || !cache.is_initialized() || minecraft_instance == nullptr ||
      cache.minecraft_client_get_render_tick_counter == nullptr ||
      cache.render_tick_counter_get_tick_progress == nullptr) {
    return 0.0f;
  }

  auto tick_counter = CallObjectMethodReference(env,
                                                minecraft_instance,
                                                cache.minecraft_client_get_render_tick_counter,
                                                kMinecraftClientClass,
                                                kMinecraftClientGetRenderTickCounterMethod,
                                                kMinecraftClientGetRenderTickCounterSignature);
  if (!tick_counter) { return 0.0f; }

  const jfloat tick_delta = env->CallFloatMethod(
      tick_counter.get(), cache.render_tick_counter_get_tick_progress, JNI_TRUE);
  if (!env->ExceptionCheck()) { return static_cast<float>(tick_delta); }

  (void)env.ClearException("call float method");
  std::println("{} jni call float method failed: {}.{} {}",
               kLogPrefix,
               kRenderTickCounterClass,
               kRenderTickCounterGetTickProgressMethod,
               kRenderTickCounterGetTickProgressSignature);
  std::fflush(stdout);
  return 0.0f;
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

JavaIterable<jobject>
ClientWorld::GetEntities(const JniEnv& env, const JniCache& cache, jobject world_instance) {
  if (!env || !cache.is_initialized() || world_instance == nullptr ||
      cache.client_world_get_entities == nullptr) {
    return {};
  }

  if (jobject entities = env->CallObjectMethod(world_instance, cache.client_world_get_entities);
      entities != nullptr) {
    if (!env->ExceptionCheck()) { return JavaIterable<jobject>(env, cache, entities); }

    env->DeleteLocalRef(entities);
  }

  if (!env->ExceptionCheck()) { return {}; }

  (void)env.ClearException("call object method");
  std::println("{} jni call object method failed: {}.{} {}",
               kLogPrefix,
               kClientWorldClass,
               kClientWorldGetEntitiesMethod,
               kClientWorldGetEntitiesSignature);
  std::fflush(stdout);
  return {};
}

ScopedLocalRef<jobject>
GameRenderer::GetCamera(const JniEnv& env, const JniCache& cache, jobject game_renderer_instance) {
  if (!env || !cache.is_initialized() || game_renderer_instance == nullptr ||
      cache.game_renderer_get_camera == nullptr) {
    return {};
  }

  return CallObjectMethodReference(env,
                                   game_renderer_instance,
                                   cache.game_renderer_get_camera,
                                   kGameRendererClass,
                                   kGameRendererGetCameraMethod,
                                   kGameRendererGetCameraSignature);
}

float GameRenderer::GetFov(const JniEnv& env,
                           const JniCache& cache,
                           jobject game_renderer_instance,
                           jobject camera_instance,
                           float tick_delta) {
  if (!env || !cache.is_initialized() || game_renderer_instance == nullptr ||
      camera_instance == nullptr || cache.game_renderer_get_fov == nullptr) {
    return 70.0f;
  }

  const jfloat fov = env->CallFloatMethod(
      game_renderer_instance, cache.game_renderer_get_fov, camera_instance, tick_delta, JNI_TRUE);
  if (!env->ExceptionCheck()) { return static_cast<float>(fov); }

  (void)env.ClearException("call float method");
  std::println("{} jni call float method failed: {}.{} {}",
               kLogPrefix,
               kGameRendererClass,
               kGameRendererGetFovMethod,
               kGameRendererGetFovSignature);
  std::fflush(stdout);
  return 70.0f;
}

Vec3 Camera::GetPosition(const JniEnv& env, const JniCache& cache, jobject camera_instance) {
  if (!env || !cache.is_initialized() || camera_instance == nullptr ||
      cache.camera_pos_field == nullptr) {
    return {};
  }

  auto camera_pos = GetObjectFieldReference(env,
                                            camera_instance,
                                            cache.camera_pos_field,
                                            kCameraClass,
                                            kCameraPosField,
                                            kCameraPosSignature);
  if (!camera_pos) { return {}; }

  return ReadVec3d(env, cache, camera_pos.get());
}

float Camera::GetPitch(const JniEnv& env, const JniCache& cache, jobject camera_instance) {
  return CallFloatMethod(env,
                         camera_instance,
                         cache.camera_get_pitch,
                         kCameraClass,
                         kCameraGetPitchMethod,
                         kCameraGetPitchSignature);
}

float Camera::GetYaw(const JniEnv& env, const JniCache& cache, jobject camera_instance) {
  return CallFloatMethod(env,
                         camera_instance,
                         cache.camera_get_yaw,
                         kCameraClass,
                         kCameraGetYawMethod,
                         kCameraGetYawSignature);
}

std::tuple<double, double, double>
Entity::GetCoordinates(const JniEnv& env, const JniCache& cache, jobject entity) {
  if (!env || !cache.is_initialized() || entity == nullptr || cache.entity_get_x == nullptr ||
      cache.entity_get_y == nullptr || cache.entity_get_z == nullptr) {
    return {0.0, 0.0, 0.0};
  }

  return {CallEntityCoordinateMethod(env, entity, cache.entity_get_x, kEntityGetXMethod),
          CallEntityCoordinateMethod(env, entity, cache.entity_get_y, kEntityGetYMethod),
          CallEntityCoordinateMethod(env, entity, cache.entity_get_z, kEntityGetZMethod)};
}

EntityData Entity::GetData(const JniEnv& env, const JniCache& cache, jobject entity) {
  EntityData data{};
  if (!env || !cache.is_initialized() || entity == nullptr) { return data; }

  // note: coordinate fetching is done separately via GetCoordinates() for performance.
  // this method only fetches the heavy data (is_alive, prev positions, height).

  data.is_alive = CallBooleanMethod(env,
                                    entity,
                                    cache.entity_is_alive,
                                    kEntityClass,
                                    kEntityIsAliveMethod,
                                    kEntityIsAliveSignature);

  auto last_render_pos = CallObjectMethodReference(env,
                                                   entity,
                                                   cache.entity_get_last_render_pos,
                                                   kEntityClass,
                                                   kEntityGetLastRenderPosMethod,
                                                   kEntityGetLastRenderPosSignature);
  if (last_render_pos) {
    const Vec3 previous_position = ReadVec3d(env, cache, last_render_pos.get());
    data.prev_x = previous_position.x;
    data.prev_y = previous_position.y;
    data.prev_z = previous_position.z;
  }

  data.height = CallFloatMethod(env,
                                entity,
                                cache.entity_get_height,
                                kEntityClass,
                                kEntityGetHeightMethod,
                                kEntityGetHeightSignature);
  return data;
}

}  // namespace mc_internal
