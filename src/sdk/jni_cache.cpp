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
  player_entity_class = CacheGlobalClass(env, kPlayerEntityClass);
  client_world_class = CacheGlobalClass(env, kClientWorldClass);
  entity_class = CacheGlobalClass(env, kEntityClass);
  game_renderer_class = CacheGlobalClass(env, kGameRendererClass);
  camera_class = CacheGlobalClass(env, kCameraClass);
  vec3d_class = CacheGlobalClass(env, kVec3dClass);
  render_tick_counter_class = CacheGlobalClass(env, kRenderTickCounterClass);
  joml_matrix4f_class = CacheGlobalClass(env, kJomlMatrix4fClass);
  java_lang_iterable_class = CacheGlobalClass(env, kJavaLangIterableClass);
  java_util_iterator_class = CacheGlobalClass(env, kJavaUtilIteratorClass);
  hostile_entity_class = CacheGlobalClass(env, kHostileEntityClass);
  passive_entity_class = CacheGlobalClass(env, kPassiveEntityClass);
  golem_entity_class = CacheGlobalClass(env, kGolemEntityClass);
  villager_entity_class = CacheGlobalClass(env, kVillagerEntityClass);
  merchant_entity_class = CacheGlobalClass(env, kMerchantEntityClass);
  water_creature_entity_class = CacheGlobalClass(env, kWaterCreatureEntityClass);
  ambient_entity_class = CacheGlobalClass(env, kAmbientEntityClass);
  animal_entity_class = CacheGlobalClass(env, kAnimalEntityClass);
  item_entity_class = CacheGlobalClass(env, kItemEntityClass);
  living_entity_class = CacheGlobalClass(env, kLivingEntityClass);

  if (minecraft_client_class == nullptr || client_player_entity_class == nullptr ||
      player_entity_class == nullptr || client_world_class == nullptr || entity_class == nullptr ||
      game_renderer_class == nullptr || camera_class == nullptr || vec3d_class == nullptr ||
      render_tick_counter_class == nullptr || joml_matrix4f_class == nullptr ||
      java_lang_iterable_class == nullptr || java_util_iterator_class == nullptr ||
      hostile_entity_class == nullptr || passive_entity_class == nullptr ||
      golem_entity_class == nullptr || villager_entity_class == nullptr ||
      merchant_entity_class == nullptr || water_creature_entity_class == nullptr ||
      ambient_entity_class == nullptr || animal_entity_class == nullptr ||
      item_entity_class == nullptr || living_entity_class == nullptr) {
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
  minecraft_client_game_renderer_field = env.GetFieldID(minecraft_client_class,
                                                        kMinecraftClientGameRendererField,
                                                        kMinecraftClientGameRendererSignature,
                                                        kMinecraftClientClass);
  minecraft_client_get_render_tick_counter =
      env.GetMethodID(minecraft_client_class,
                      kMinecraftClientGetRenderTickCounterMethod,
                      kMinecraftClientGetRenderTickCounterSignature,
                      kMinecraftClientClass);
  client_world_get_entities = env.GetMethodID(client_world_class,
                                              kClientWorldGetEntitiesMethod,
                                              kClientWorldGetEntitiesSignature,
                                              kClientWorldClass);
  game_renderer_get_camera = env.GetMethodID(game_renderer_class,
                                             kGameRendererGetCameraMethod,
                                             kGameRendererGetCameraSignature,
                                             kGameRendererClass);
  game_renderer_get_fov = env.GetMethodID(game_renderer_class,
                                          kGameRendererGetFovMethod,
                                          kGameRendererGetFovSignature,
                                          kGameRendererClass);
  camera_pos_field =
      env.GetFieldID(camera_class, kCameraPosField, kCameraPosSignature, kCameraClass);
  camera_get_pitch =
      env.GetMethodID(camera_class, kCameraGetPitchMethod, kCameraGetPitchSignature, kCameraClass);
  camera_get_yaw =
      env.GetMethodID(camera_class, kCameraGetYawMethod, kCameraGetYawSignature, kCameraClass);
  render_tick_counter_get_tick_progress =
      env.GetMethodID(render_tick_counter_class,
                      kRenderTickCounterGetTickProgressMethod,
                      kRenderTickCounterGetTickProgressSignature,
                      kRenderTickCounterClass);
  vec3d_x_field = env.GetFieldID(vec3d_class, kVec3dGetXField, kVec3dCoordSignature, kVec3dClass);
  vec3d_y_field = env.GetFieldID(vec3d_class, kVec3dGetYField, kVec3dCoordSignature, kVec3dClass);
  vec3d_z_field = env.GetFieldID(vec3d_class, kVec3dGetZField, kVec3dCoordSignature, kVec3dClass);
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
  entity_is_alive =
      env.GetMethodID(entity_class, kEntityIsAliveMethod, kEntityIsAliveSignature, kEntityClass);
  entity_get_last_render_pos = env.GetMethodID(
      entity_class, kEntityGetLastRenderPosMethod, kEntityGetLastRenderPosSignature, kEntityClass);
  entity_get_height = env.GetMethodID(
      entity_class, kEntityGetHeightMethod, kEntityGetHeightSignature, kEntityClass);
  entity_get_width =
      env.GetMethodID(entity_class, kEntityGetWidthMethod, kEntityGetWidthSignature, kEntityClass);
  entity_get_type =
      env.GetMethodID(entity_class, kEntityGetTypeMethod, kEntityGetTypeSignature, kEntityClass);
  entity_get_name =
      env.GetMethodID(entity_class, kEntityGetNameMethod, kEntityGetNameSignature, kEntityClass);
  entity_get_id =
      env.GetMethodID(entity_class, kEntityGetIdMethod, kEntityGetIdSignature, kEntityClass);

  living_entity_get_health = env.GetMethodID(living_entity_class,
                                             kLivingEntityGetHealthMethod,
                                             kLivingEntityGetHealthSignature,
                                             kLivingEntityClass);
  living_entity_get_max_health = env.GetMethodID(living_entity_class,
                                                 kLivingEntityGetMaxHealthMethod,
                                                 kLivingEntityGetMaxHealthSignature,
                                                 kLivingEntityClass);
  living_entity_get_absorption_amount = env.GetMethodID(living_entity_class,
                                                        kLivingEntityGetAbsorptionAmountMethod,
                                                        kLivingEntityGetAbsorptionAmountSignature,
                                                        kLivingEntityClass);

  {
    auto entity_type_class = CacheGlobalClass(env, kEntityTypeClass);
    if (entity_type_class != nullptr) {
      entity_type_translation_key_field = env.GetFieldID(entity_type_class,
                                                         kEntityTypeTranslationKeyField,
                                                         kEntityTypeTranslationKeyFieldSignature,
                                                         kEntityTypeClass);
      env->DeleteGlobalRef(entity_type_class);
    }
  }

  {
    auto text_class = CacheGlobalClass(env, kTextClass);
    if (text_class != nullptr) {
      text_get_string =
          env.GetMethodID(text_class, kTextGetStringMethod, kTextGetStringSignature, kTextClass);
      env->DeleteGlobalRef(text_class);
    }
  }

  player_entity_get_game_profile = env.GetMethodID(player_entity_class,
                                                   kPlayerEntityGetGameProfileMethod,
                                                   kPlayerEntityGetGameProfileSignature,
                                                   kPlayerEntityClass);
  if (player_entity_get_game_profile != nullptr) {
    auto game_profile_class = CacheGlobalClass(env, kGameProfileClass);
    if (game_profile_class != nullptr) {
      game_profile_get_name = env.GetMethodID(game_profile_class,
                                              kGameProfileGetNameMethod,
                                              kGameProfileGetNameSignature,
                                              kGameProfileClass);
      env->DeleteGlobalRef(game_profile_class);
    }
  }

  if (minecraft_client_get_instance == nullptr || minecraft_client_player_field == nullptr ||
      minecraft_client_world_field == nullptr || minecraft_client_game_renderer_field == nullptr ||
      minecraft_client_get_render_tick_counter == nullptr || client_world_get_entities == nullptr ||
      entity_is_alive == nullptr || entity_get_height == nullptr || entity_get_width == nullptr ||
      entity_get_last_render_pos == nullptr || game_renderer_get_camera == nullptr ||
      game_renderer_get_fov == nullptr || camera_pos_field == nullptr ||
      camera_get_pitch == nullptr || camera_get_yaw == nullptr ||
      render_tick_counter_get_tick_progress == nullptr || vec3d_x_field == nullptr ||
      vec3d_y_field == nullptr || vec3d_z_field == nullptr || entity_get_x == nullptr ||
      entity_get_y == nullptr || entity_get_z == nullptr || iterable_iterator == nullptr ||
      iterator_has_next == nullptr || iterator_next == nullptr || entity_get_type == nullptr ||
      living_entity_get_health == nullptr || living_entity_get_max_health == nullptr ||
      living_entity_get_absorption_amount == nullptr) {
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
  if (env != nullptr && player_entity_class != nullptr) {
    env->DeleteGlobalRef(player_entity_class);
  }
  if (env != nullptr && client_world_class != nullptr) { env->DeleteGlobalRef(client_world_class); }
  if (env != nullptr && entity_class != nullptr) { env->DeleteGlobalRef(entity_class); }
  if (env != nullptr && game_renderer_class != nullptr) {
    env->DeleteGlobalRef(game_renderer_class);
  }
  if (env != nullptr && camera_class != nullptr) { env->DeleteGlobalRef(camera_class); }
  if (env != nullptr && vec3d_class != nullptr) { env->DeleteGlobalRef(vec3d_class); }
  if (env != nullptr && render_tick_counter_class != nullptr) {
    env->DeleteGlobalRef(render_tick_counter_class);
  }
  if (env != nullptr && joml_matrix4f_class != nullptr) {
    env->DeleteGlobalRef(joml_matrix4f_class);
  }
  if (env != nullptr && java_lang_iterable_class != nullptr) {
    env->DeleteGlobalRef(java_lang_iterable_class);
  }
  if (env != nullptr && java_util_iterator_class != nullptr) {
    env->DeleteGlobalRef(java_util_iterator_class);
  }
  if (env != nullptr && hostile_entity_class != nullptr) {
    env->DeleteGlobalRef(hostile_entity_class);
  }
  if (env != nullptr && passive_entity_class != nullptr) {
    env->DeleteGlobalRef(passive_entity_class);
  }
  if (env != nullptr && golem_entity_class != nullptr) { env->DeleteGlobalRef(golem_entity_class); }
  if (env != nullptr && villager_entity_class != nullptr) {
    env->DeleteGlobalRef(villager_entity_class);
  }
  if (env != nullptr && merchant_entity_class != nullptr) {
    env->DeleteGlobalRef(merchant_entity_class);
  }
  if (env != nullptr && water_creature_entity_class != nullptr) {
    env->DeleteGlobalRef(water_creature_entity_class);
  }
  if (env != nullptr && ambient_entity_class != nullptr) {
    env->DeleteGlobalRef(ambient_entity_class);
  }
  if (env != nullptr && animal_entity_class != nullptr) {
    env->DeleteGlobalRef(animal_entity_class);
  }
  if (env != nullptr && item_entity_class != nullptr) { env->DeleteGlobalRef(item_entity_class); }
  if (env != nullptr && living_entity_class != nullptr) {
    env->DeleteGlobalRef(living_entity_class);
  }

  minecraft_client_class = nullptr;
  client_player_entity_class = nullptr;
  player_entity_class = nullptr;
  client_world_class = nullptr;
  entity_class = nullptr;
  game_renderer_class = nullptr;
  camera_class = nullptr;
  vec3d_class = nullptr;
  render_tick_counter_class = nullptr;
  joml_matrix4f_class = nullptr;
  java_lang_iterable_class = nullptr;
  java_util_iterator_class = nullptr;
  hostile_entity_class = nullptr;
  passive_entity_class = nullptr;
  golem_entity_class = nullptr;
  villager_entity_class = nullptr;
  merchant_entity_class = nullptr;
  water_creature_entity_class = nullptr;
  ambient_entity_class = nullptr;
  animal_entity_class = nullptr;
  item_entity_class = nullptr;
  living_entity_class = nullptr;

  minecraft_client_get_instance = nullptr;
  minecraft_client_player_field = nullptr;
  minecraft_client_world_field = nullptr;
  minecraft_client_game_renderer_field = nullptr;
  minecraft_client_get_render_tick_counter = nullptr;
  client_world_get_entities = nullptr;
  entity_get_x = nullptr;
  entity_get_y = nullptr;
  entity_get_z = nullptr;
  entity_is_alive = nullptr;
  entity_get_last_render_pos = nullptr;
  entity_get_height = nullptr;
  entity_get_width = nullptr;
  entity_get_type = nullptr;
  entity_get_name = nullptr;
  entity_type_translation_key_field = nullptr;
  living_entity_get_health = nullptr;
  living_entity_get_max_health = nullptr;
  living_entity_get_absorption_amount = nullptr;
  entity_get_id = nullptr;
  player_entity_get_game_profile = nullptr;
  game_profile_get_name = nullptr;
  text_get_string = nullptr;
  game_renderer_get_camera = nullptr;
  game_renderer_get_fov = nullptr;
  camera_get_pitch = nullptr;
  camera_get_yaw = nullptr;
  render_tick_counter_get_tick_progress = nullptr;
  iterable_iterator = nullptr;
  iterator_has_next = nullptr;
  iterator_next = nullptr;
  camera_pos_field = nullptr;
  vec3d_x_field = nullptr;
  vec3d_y_field = nullptr;
  vec3d_z_field = nullptr;

  initialized_ = false;
}

}  // namespace mc_internal
