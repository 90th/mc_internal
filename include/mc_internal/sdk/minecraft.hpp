#pragma once

#include <string>
#include <string_view>
#include <tuple>
#include <unordered_set>

#include "mc_internal/sdk/jni_cache.hpp"
#include "mc_internal/sdk/jni_env.hpp"
#include "mc_internal/sdk/java_iterator.hpp"
#include "mc_internal/sdk/math.hpp"
#include "mc_internal/sdk/mappings.hpp"

namespace mc_internal {

struct EntityData {
  bool is_alive = false;
  double x = 0.0;
  double y = 0.0;
  double z = 0.0;
  double prev_x = 0.0;
  double prev_y = 0.0;
  double prev_z = 0.0;
  float height = 0.0f;
  float width = 0.0f;
};

class Minecraft {
 public:
  [[nodiscard]] static ScopedLocalRef<jobject> GetInstance(const JniEnv& env,
                                                           const JniCache& cache);
  [[nodiscard]] static ScopedLocalRef<jobject>
  GetLocalPlayer(const JniEnv& env, const JniCache& cache, jobject minecraft_instance);
  [[nodiscard]] static ScopedLocalRef<jobject>
  GetWorld(const JniEnv& env, const JniCache& cache, jobject minecraft_instance);
  [[nodiscard]] static ScopedLocalRef<jobject>
  GetGameRenderer(const JniEnv& env, const JniCache& cache, jobject minecraft_instance);
  [[nodiscard]] static float
  GetTickDelta(const JniEnv& env, const JniCache& cache, jobject minecraft_instance);
};

class ClientPlayerEntity {
 public:
  [[nodiscard]] static std::tuple<double, double, double>
  GetCoordinates(const JniEnv& env, const JniCache& cache, jobject player);
};

class ClientWorld {
 public:
  [[nodiscard]] static ScopedLocalRef<jclass> FindClass(const JniEnv& env, const JniCache& cache);
  [[nodiscard]] static JavaIterable<jobject>
  GetEntities(const JniEnv& env, const JniCache& cache, jobject world_instance);
};

class GameRenderer {
 public:
  [[nodiscard]] static ScopedLocalRef<jobject>
  GetCamera(const JniEnv& env, const JniCache& cache, jobject game_renderer_instance);
  [[nodiscard]] static float GetFov(const JniEnv& env,
                                    const JniCache& cache,
                                    jobject game_renderer_instance,
                                    jobject camera_instance,
                                    float tick_delta);
};

class Camera {
 public:
  [[nodiscard]] static Vec3
  GetPosition(const JniEnv& env, const JniCache& cache, jobject camera_instance);
  [[nodiscard]] static float
  GetPitch(const JniEnv& env, const JniCache& cache, jobject camera_instance);
  [[nodiscard]] static float
  GetYaw(const JniEnv& env, const JniCache& cache, jobject camera_instance);
};

class Entity {
 public:
  [[nodiscard]] static bool IsAlive(const JniEnv& env, const JniCache& cache, jobject entity);
  [[nodiscard]] static EntityData GetData(const JniEnv& env, const JniCache& cache, jobject entity);
  [[nodiscard]] static std::tuple<double, double, double>
  GetCoordinates(const JniEnv& env, const JniCache& cache, jobject entity);
  [[nodiscard]] static std::string
  GetTranslationKey(const JniEnv& env, const JniCache& cache, jobject entity);
  [[nodiscard]] static bool IsTranslationKeyInSet(const JniEnv& env,
                                                  const JniCache& cache,
                                                  jobject entity,
                                                  const std::unordered_set<std::string_view>& keys);
  [[nodiscard]] static std::string
  GetName(const JniEnv& env, const JniCache& cache, jobject entity);
  [[nodiscard]] static int GetId(const JniEnv& env, const JniCache& cache, jobject entity);
};

class LivingEntity {
 public:
  [[nodiscard]] static float GetHealth(const JniEnv& env, const JniCache& cache, jobject entity);
  [[nodiscard]] static float GetMaxHealth(const JniEnv& env, const JniCache& cache, jobject entity);
  [[nodiscard]] static float
  GetAbsorptionAmount(const JniEnv& env, const JniCache& cache, jobject entity);
};

}  // namespace mc_internal
