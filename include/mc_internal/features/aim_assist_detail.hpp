#pragma once

#include <limits>

#include "mc_internal/sdk/math.hpp"

namespace mc_internal::aim_assist_detail {

struct AimPoint {
  float yaw = 0.0f;
  float pitch = 0.0f;
  float fov_delta = std::numeric_limits<float>::max();
  double distance_sq = 0.0;
};

struct TargetSearchPlan {
  bool prefer_locked_target = false;
  int requested_target_id = 0;
  bool retry_without_lock = false;
};

[[nodiscard]] bool IsFinite(float value);
[[nodiscard]] bool IsFinite(double value);
[[nodiscard]] bool IsFinite(const Vec3& value);
[[nodiscard]] bool IsAimPointValid(const AimPoint& point);
[[nodiscard]] bool IsRuntimeConfigValid(float fov_degrees, float smoothness, float max_distance);
[[nodiscard]] bool CanResolveHostileTargetKeys(bool target_hostiles_enabled,
                                               bool has_entity_type_method,
                                               bool has_translation_key_field);
[[nodiscard]] bool CanUseLockedTargetFastPath(const TargetSearchPlan& plan,
                                              bool has_entity_lookup_method);
[[nodiscard]] bool ShouldContinueFovSampling(float best_fov_delta);
[[nodiscard]] bool PassesHardFov(const AimPoint& point, float fov_limit);
[[nodiscard]] float WrapDegrees(float value);
[[nodiscard]] float ClampPitch(float pitch);
[[nodiscard]] Vec3 ViewDirection(float yaw, float pitch);
[[nodiscard]] Vec3 ClampHorizontalVelocity(Vec3 velocity, double max_speed);
[[nodiscard]] Vec3 PredictPosition(const Vec3& position, const Vec3& velocity, double lead_ticks);
[[nodiscard]] float SmartPredictionStrength(double distance,
                                            double max_distance,
                                            const Vec3& relative_velocity,
                                            const Vec3& target_delta);
[[nodiscard]] double
PredictionLeadTicks(float prediction_strength, float smoothness, float delta_time);
[[nodiscard]] bool
RayIntersectsAabb(const Vec3& origin, const Vec3& direction, const Vec3& min, const Vec3& max);
[[nodiscard]] AimPoint BuildAimPoint(const Vec3& camera_position,
                                     float current_yaw,
                                     float current_pitch,
                                     const Vec3& point);
[[nodiscard]] TargetSearchPlan
PlanTargetSearch(bool activation_was_down, bool has_locked_target, int locked_target_id);

}  // namespace mc_internal::aim_assist_detail
