#include "mc_internal/features/aim_assist_detail.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace mc_internal::aim_assist_detail {
namespace {

constexpr float kPi = 3.14159265358979323846f;

Vec3 InvalidVec3() {
  const double nan = std::numeric_limits<double>::quiet_NaN();
  return {nan, nan, nan};
}

double HorizontalSpeed(const Vec3& velocity) {
  if (!IsFinite(velocity)) { return std::numeric_limits<double>::quiet_NaN(); }
  return std::sqrt(velocity.x * velocity.x + velocity.z * velocity.z);
}

double LateralSpeed(const Vec3& relative_velocity, const Vec3& target_delta) {
  if (!IsFinite(relative_velocity) || !IsFinite(target_delta)) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  const double distance_sq = target_delta.x * target_delta.x + target_delta.z * target_delta.z;
  if (distance_sq <= 0.000001) { return HorizontalSpeed(relative_velocity); }

  const double distance = std::sqrt(distance_sq);
  const double forward_x = target_delta.x / distance;
  const double forward_z = target_delta.z / distance;
  const double forward_speed = relative_velocity.x * forward_x + relative_velocity.z * forward_z;
  const double speed_sq =
      relative_velocity.x * relative_velocity.x + relative_velocity.z * relative_velocity.z;
  return std::sqrt(std::max(speed_sq - forward_speed * forward_speed, 0.0));
}

float SmoothStep(float value) {
  if (!IsFinite(value)) { return 0.0f; }
  value = std::clamp(value, 0.0f, 1.0f);
  return value * value * (3.0f - 2.0f * value);
}

float ToDegrees(float radians) { return radians * (180.0f / kPi); }

float ToRadians(float degrees) { return degrees * (kPi / 180.0f); }

}  // namespace

bool IsFinite(float value) { return std::isfinite(value); }

bool IsFinite(double value) { return std::isfinite(value); }

bool IsFinite(const Vec3& value) {
  return IsFinite(value.x) && IsFinite(value.y) && IsFinite(value.z);
}

bool IsAimPointValid(const AimPoint& point) {
  return IsFinite(point.yaw) && IsFinite(point.pitch) && IsFinite(point.fov_delta) &&
         IsFinite(point.distance_sq) && point.distance_sq > 0.0001 && point.fov_delta >= 0.0f;
}

bool IsRuntimeConfigValid(float fov_degrees, float smoothness, float max_distance) {
  return IsFinite(fov_degrees) && IsFinite(smoothness) && IsFinite(max_distance) &&
         smoothness > 0.0f && max_distance > 0.0f;
}

bool CanResolveHostileTargetKeys(bool target_hostiles_enabled,
                                 bool has_entity_type_method,
                                 bool has_translation_key_field) {
  return !target_hostiles_enabled || (has_entity_type_method && has_translation_key_field);
}

bool CanUseLockedTargetFastPath(const TargetSearchPlan& plan, bool has_entity_lookup_method) {
  return plan.prefer_locked_target && has_entity_lookup_method;
}

bool ShouldContinueFovSampling(float best_fov_delta) {
  return !IsFinite(best_fov_delta) || best_fov_delta > 0.0f;
}

bool PassesHardFov(const AimPoint& point, float fov_limit) {
  return IsAimPointValid(point) && IsFinite(fov_limit) && fov_limit > 0.0f &&
         point.fov_delta <= fov_limit;
}

TargetSearchPlan
PlanTargetSearch(bool activation_was_down, bool has_locked_target, int locked_target_id) {
  if (!activation_was_down || !has_locked_target) { return {}; }
  return {.prefer_locked_target = true,
          .requested_target_id = locked_target_id,
          .retry_without_lock = true};
}

float WrapDegrees(float value) {
  if (!IsFinite(value)) { return 0.0f; }
  while (value <= -180.0f) value += 360.0f;
  while (value > 180.0f) value -= 360.0f;
  return value;
}

float ClampPitch(float pitch) {
  if (!IsFinite(pitch)) { return 0.0f; }
  return std::clamp(pitch, -89.0f, 89.0f);
}

Vec3 ViewDirection(float yaw, float pitch) {
  if (!IsFinite(yaw) || !IsFinite(pitch)) { return InvalidVec3(); }
  const float yaw_radians = ToRadians(yaw);
  const float pitch_radians = ToRadians(pitch);
  const double pitch_cos = std::cos(pitch_radians);
  return {-std::sin(yaw_radians) * pitch_cos,
          -std::sin(pitch_radians),
          std::cos(yaw_radians) * pitch_cos};
}

Vec3 ClampHorizontalVelocity(Vec3 velocity, double max_speed) {
  if (!IsFinite(velocity) || !IsFinite(max_speed)) { return InvalidVec3(); }
  velocity.y = 0.0;
  if (max_speed <= 0.0) { return velocity; }
  const double speed_sq = velocity.x * velocity.x + velocity.z * velocity.z;
  const double max_speed_sq = max_speed * max_speed;
  if (speed_sq > max_speed_sq && speed_sq > 0.000001) {
    const double scale = max_speed / std::sqrt(speed_sq);
    velocity.x *= scale;
    velocity.z *= scale;
  }
  return velocity;
}

Vec3 PredictPosition(const Vec3& position, const Vec3& velocity, double lead_ticks) {
  if (!IsFinite(position) || !IsFinite(velocity) || !IsFinite(lead_ticks)) { return InvalidVec3(); }
  return {position.x + velocity.x * lead_ticks,
          position.y + velocity.y * lead_ticks,
          position.z + velocity.z * lead_ticks};
}

float SmartPredictionStrength(double distance,
                              double max_distance,
                              const Vec3& relative_velocity,
                              const Vec3& target_delta) {
  constexpr float kMinPrediction = 0.95f;
  constexpr float kMaxPrediction = 1.0f;
  if (!IsFinite(distance) || !IsFinite(max_distance) || !IsFinite(relative_velocity) ||
      !IsFinite(target_delta) || max_distance <= 0.0) {
    return 0.0f;
  }

  const float distance_t = std::clamp(
      static_cast<float>((distance - 8.0) / std::max(max_distance * 0.55, 1.0)), 0.0f, 1.0f);
  const float lateral_speed_t = std::clamp(
      static_cast<float>(LateralSpeed(relative_velocity, target_delta) / 0.45), 0.0f, 1.0f);
  const float blend_t = SmoothStep(lateral_speed_t * 0.80f + distance_t * 0.20f);
  return kMinPrediction + (kMaxPrediction - kMinPrediction) * blend_t;
}

double PredictionLeadTicks(float prediction_strength, float smoothness, float delta_time) {
  if (!IsFinite(prediction_strength) || !IsFinite(smoothness) || !IsFinite(delta_time) ||
      prediction_strength <= 0.0f || smoothness <= 0.0f) {
    return 0.0;
  }

  const float clamped_delta_time = std::clamp(delta_time, 1.0f / 240.0f, 1.0f / 20.0f);
  const double response_ticks =
      std::clamp(static_cast<double>(clamped_delta_time) * 20.0 * smoothness, 0.25, 4.0);
  return response_ticks * static_cast<double>(std::clamp(prediction_strength, 0.0f, 1.0f));
}

bool RayIntersectsAabb(const Vec3& origin,
                       const Vec3& direction,
                       const Vec3& min,
                       const Vec3& max) {
  if (!IsFinite(origin) || !IsFinite(direction) || !IsFinite(min) || !IsFinite(max)) {
    return false;
  }

  double t_min = 0.0;
  double t_max = std::numeric_limits<double>::max();

  const auto update_axis =
      [&](double origin_value, double direction_value, double min_value, double max_value) {
        if (std::abs(direction_value) < 0.000001) {
          return origin_value >= min_value && origin_value <= max_value;
        }

        double near_t = (min_value - origin_value) / direction_value;
        double far_t = (max_value - origin_value) / direction_value;
        if (near_t > far_t) { std::swap(near_t, far_t); }
        t_min = std::max(t_min, near_t);
        t_max = std::min(t_max, far_t);
        return t_min <= t_max;
      };

  return update_axis(origin.x, direction.x, min.x, max.x) &&
         update_axis(origin.y, direction.y, min.y, max.y) &&
         update_axis(origin.z, direction.z, min.z, max.z) && t_max >= 0.0;
}

AimPoint BuildAimPoint(const Vec3& camera_position,
                       float current_yaw,
                       float current_pitch,
                       const Vec3& point) {
  if (!IsFinite(camera_position) || !IsFinite(current_yaw) || !IsFinite(current_pitch) ||
      !IsFinite(point)) {
    return {};
  }

  const double dx = point.x - camera_position.x;
  const double dy = point.y - camera_position.y;
  const double dz = point.z - camera_position.z;
  const double distance_sq = dx * dx + dy * dy + dz * dz;
  if (!IsFinite(distance_sq) || distance_sq <= 0.0001) { return {}; }

  const double horiz = std::sqrt(dx * dx + dz * dz);
  if (!IsFinite(horiz)) { return {}; }

  const float yaw = WrapDegrees(ToDegrees(std::atan2(-dx, dz)));
  const float pitch = ClampPitch(ToDegrees(static_cast<float>(-std::atan2(dy, horiz))));
  const float yaw_delta = WrapDegrees(yaw - current_yaw);
  const float pitch_delta = pitch - current_pitch;
  const float fov_delta = std::sqrt(yaw_delta * yaw_delta + pitch_delta * pitch_delta);
  if (!IsFinite(yaw) || !IsFinite(pitch) || !IsFinite(fov_delta)) { return {}; }

  return {yaw, pitch, fov_delta, distance_sq};
}

}  // namespace mc_internal::aim_assist_detail
