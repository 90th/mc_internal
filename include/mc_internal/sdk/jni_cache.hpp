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
  jclass player_entity_class = nullptr;
  jclass client_world_class = nullptr;
  jclass entity_class = nullptr;
  jclass game_renderer_class = nullptr;
  jclass camera_class = nullptr;
  jclass vec3d_class = nullptr;
  jclass render_tick_counter_class = nullptr;
  jclass joml_matrix4f_class = nullptr;
  jclass java_lang_iterable_class = nullptr;
  jclass java_util_iterator_class = nullptr;
  jclass hostile_entity_class = nullptr;
  jclass passive_entity_class = nullptr;
  jclass golem_entity_class = nullptr;
  jclass villager_entity_class = nullptr;
  jclass merchant_entity_class = nullptr;
  jclass water_creature_entity_class = nullptr;
  jclass ambient_entity_class = nullptr;
  jclass animal_entity_class = nullptr;
  jclass item_entity_class = nullptr;
  jclass living_entity_class = nullptr;

  jmethodID minecraft_client_get_instance = nullptr;
  jfieldID minecraft_client_player_field = nullptr;
  jfieldID minecraft_client_world_field = nullptr;
  jfieldID minecraft_client_game_renderer_field = nullptr;
  jmethodID minecraft_client_get_render_tick_counter = nullptr;
  jmethodID client_world_get_entities = nullptr;

  jmethodID entity_get_x = nullptr;
  jmethodID entity_get_y = nullptr;
  jmethodID entity_get_z = nullptr;
  jmethodID entity_is_alive = nullptr;
  jmethodID entity_get_last_render_pos = nullptr;
  jmethodID entity_get_height = nullptr;
  jmethodID entity_get_width = nullptr;
  jmethodID entity_get_type = nullptr;
  jmethodID entity_get_name = nullptr;
  jmethodID entity_get_id = nullptr;
  jfieldID entity_type_translation_key_field = nullptr;
  jmethodID living_entity_get_health = nullptr;
  jmethodID living_entity_get_max_health = nullptr;
  jmethodID living_entity_get_absorption_amount = nullptr;
  jmethodID player_entity_get_game_profile = nullptr;
  jmethodID game_profile_get_name = nullptr;
  jmethodID text_get_string = nullptr;
  jmethodID game_renderer_get_camera = nullptr;
  jmethodID game_renderer_get_fov = nullptr;
  jmethodID camera_get_pitch = nullptr;
  jmethodID camera_get_yaw = nullptr;
  jmethodID render_tick_counter_get_tick_progress = nullptr;
  jmethodID iterable_iterator = nullptr;
  jmethodID iterator_has_next = nullptr;
  jmethodID iterator_next = nullptr;
  jfieldID camera_pos_field = nullptr;
  jfieldID vec3d_x_field = nullptr;
  jfieldID vec3d_y_field = nullptr;
  jfieldID vec3d_z_field = nullptr;

 private:
  void Reset(JNIEnv* env) noexcept;

  bool initialized_ = false;
};

}  // namespace mc_internal
