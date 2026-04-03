#pragma once

#include <tuple>

#include "mc_internal/sdk/jni_cache.hpp"
#include "mc_internal/sdk/jni_env.hpp"
#include "mc_internal/sdk/mappings.hpp"

namespace mc_internal {

class Minecraft {
 public:
  [[nodiscard]] static ScopedLocalRef<jobject> GetInstance(const JniEnv& env,
                                                           const JniCache& cache);
  [[nodiscard]] static ScopedLocalRef<jobject>
  GetLocalPlayer(const JniEnv& env, const JniCache& cache, jobject minecraft_instance);
};

class ClientPlayerEntity {
 public:
  [[nodiscard]] static std::tuple<double, double, double>
  GetCoordinates(const JniEnv& env, const JniCache& cache, jobject player);
};

class ClientWorld {
 public:
  [[nodiscard]] static ScopedLocalRef<jclass> FindClass(const JniEnv& env, const JniCache& cache);
  [[nodiscard]] static jobject
  GetEntities(const JniEnv& env, const JniCache& cache, jobject world_instance);
};

}  // namespace mc_internal
