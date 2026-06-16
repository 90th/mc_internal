#include "mc_internal/features/aim_assist_module.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

#include "imgui.h"

#include "mc_internal/context.hpp"
#include "mc_internal/sdk/minecraft.hpp"
#include "mc_internal/ui/widgets.hpp"

namespace mc_internal {

namespace {

constexpr float kPi = 3.14159265358979323846f;
constexpr std::array<std::string_view, 37> kHostileKeys = {
    "entity.minecraft.blaze",
    "entity.minecraft.bogged",
    "entity.minecraft.breeze",
    "entity.minecraft.cave_spider",
    "entity.minecraft.creaking",
    "entity.minecraft.creeper",
    "entity.minecraft.drowned",
    "entity.minecraft.elder_guardian",
    "entity.minecraft.ender_dragon",
    "entity.minecraft.enderman",
    "entity.minecraft.endermite",
    "entity.minecraft.evoker",
    "entity.minecraft.ghast",
    "entity.minecraft.guardian",
    "entity.minecraft.hoglin",
    "entity.minecraft.husk",
    "entity.minecraft.magma_cube",
    "entity.minecraft.phantom",
    "entity.minecraft.piglin_brute",
    "entity.minecraft.pillager",
    "entity.minecraft.ravager",
    "entity.minecraft.shulker",
    "entity.minecraft.silverfish",
    "entity.minecraft.skeleton",
    "entity.minecraft.slime",
    "entity.minecraft.spider",
    "entity.minecraft.stray",
    "entity.minecraft.vex",
    "entity.minecraft.vindicator",
    "entity.minecraft.warden",
    "entity.minecraft.witch",
    "entity.minecraft.wither",
    "entity.minecraft.wither_skeleton",
    "entity.minecraft.zoglin",
    "entity.minecraft.zombie",
    "entity.minecraft.zombie_villager",
    "entity.minecraft.zombified_piglin",
};

bool IsTrackedHostile(std::string_view key) {
  return std::find(kHostileKeys.begin(), kHostileKeys.end(), key) != kHostileKeys.end();
}

float ToDegrees(float radians) { return radians * (180.0f / kPi); }

float ToRadians(float degrees) { return degrees * (kPi / 180.0f); }

float WrapDegrees(float value) {
  while (value <= -180.0f) value += 360.0f;
  while (value > 180.0f) value -= 360.0f;
  return value;
}

float ClampPitch(float pitch) { return std::clamp(pitch, -89.0f, 89.0f); }

Vec3 ViewDirection(float yaw, float pitch) {
  const float yaw_radians = ToRadians(yaw);
  const float pitch_radians = ToRadians(pitch);
  const double pitch_cos = std::cos(pitch_radians);
  return {-std::sin(yaw_radians) * pitch_cos,
          -std::sin(pitch_radians),
          std::cos(yaw_radians) * pitch_cos};
}

Vec3 ClampHorizontalVelocity(Vec3 velocity, double max_speed) {
  velocity.y = 0.0;
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
  return {position.x + velocity.x * lead_ticks,
          position.y + velocity.y * lead_ticks,
          position.z + velocity.z * lead_ticks};
}

double HorizontalSpeed(const Vec3& velocity) {
  return std::sqrt(velocity.x * velocity.x + velocity.z * velocity.z);
}

double LateralSpeed(const Vec3& relative_velocity, const Vec3& target_delta) {
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
  value = std::clamp(value, 0.0f, 1.0f);
  return value * value * (3.0f - 2.0f * value);
}

float SmartPredictionStrength(double distance,
                              double max_distance,
                              const Vec3& relative_velocity,
                              const Vec3& target_delta) {
  constexpr float kMinPrediction = 0.95f;
  constexpr float kMaxPrediction = 1.0f;

  const float distance_t = std::clamp(
      static_cast<float>((distance - 8.0) / std::max(max_distance * 0.55, 1.0)), 0.0f, 1.0f);
  const float lateral_speed_t = std::clamp(
      static_cast<float>(LateralSpeed(relative_velocity, target_delta) / 0.45), 0.0f, 1.0f);
  const float blend_t = SmoothStep(lateral_speed_t * 0.80f + distance_t * 0.20f);
  return kMinPrediction + (kMaxPrediction - kMinPrediction) * blend_t;
}

double PredictionLeadTicks(float prediction_strength, float smoothness) {
  if (prediction_strength <= 0.0f) { return 0.0; }

  const float delta_time = std::clamp(ImGui::GetIO().DeltaTime, 1.0f / 240.0f, 1.0f / 20.0f);
  const double response_ticks =
      std::clamp(static_cast<double>(delta_time) * 20.0 * smoothness, 0.25, 4.0);
  return response_ticks * static_cast<double>(std::clamp(prediction_strength, 0.0f, 1.0f));
}

bool RayIntersectsAabb(const Vec3& origin,
                       const Vec3& direction,
                       const Vec3& min,
                       const Vec3& max) {
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

struct AimPoint {
  float yaw = 0.0f;
  float pitch = 0.0f;
  float fov_delta = std::numeric_limits<float>::max();
  double distance_sq = 0.0;
};

AimPoint BuildAimPoint(const Vec3& camera_position,
                       float current_yaw,
                       float current_pitch,
                       const Vec3& point) {
  const double dx = point.x - camera_position.x;
  const double dy = point.y - camera_position.y;
  const double dz = point.z - camera_position.z;
  const double distance_sq = dx * dx + dy * dy + dz * dz;
  if (distance_sq <= 0.0001) { return {}; }

  const double horiz = std::sqrt(dx * dx + dz * dz);
  const float yaw = WrapDegrees(ToDegrees(std::atan2(-dx, dz)));
  const float pitch = ClampPitch(ToDegrees(static_cast<float>(-std::atan2(dy, horiz))));
  const float yaw_delta = WrapDegrees(yaw - current_yaw);
  const float pitch_delta = pitch - current_pitch;
  return {yaw, pitch, std::sqrt(yaw_delta * yaw_delta + pitch_delta * pitch_delta), distance_sq};
}

bool IsAttackHeld(const OverlayContext& ctx) {
  return !ctx.show_menu && ctx.pinned_window != nullptr && ctx.glfw.get_mouse_button != nullptr &&
         ctx.glfw.get_mouse_button(ctx.pinned_window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
}

}  // namespace

AimAssistModule::AimAssistModule()
    : Module("aim assist",
             "smoothly nudges aim toward nearby targets while attacking",
             ModuleCategory::kCombat) {}

void AimAssistModule::on_render_settings(const OverlayContext& ctx) {
  static_cast<void>(ctx);

  ui::SectionHeader("targeting");
  ui::DescriptionText(
      "nudges your crosshair toward the closest target inside the configured field of view.");
  ImGui::Spacing();

  ui::Toggle("target players", &target_players_);
  ui::Toggle("target hostiles", &target_hostiles_);
  ui::Toggle("line of sight only", &line_of_sight_only_);

  ui::SectionHeader("tuning");
  ui::DescriptionText(
      "higher smoothness slows the correction and keeps the movement looking more human. target "
      "choice blends crosshair error with distance. prediction automatically adapts to movement.");
  ImGui::Spacing();

  ui::LabeledSlider("field of view", "##aim_assist_fov", &fov_degrees_, 1.0f, 180.0f, "%.0f deg");
  ui::LabeledSlider("smoothness", "##aim_assist_smooth", &smoothness_, 1.0f, 20.0f, "%.1f");
  ui::LabeledSlider(
      "max distance", "##aim_assist_distance", &max_distance_, 8.0f, 192.0f, "%.0f blocks");
}

void AimAssistModule::on_render_3d(const OverlayContext& ctx) {
  if ((!target_players_ && !target_hostiles_) || !IsAttackHeld(ctx) || !ctx.jvm_attachment) {
    return;
  }

  const JniEnv env(ctx.jvm_attachment->env());
  if (!ctx.jni_cache.Initialize(env)) { return; }

  auto minecraft_instance = Minecraft::GetInstance(env, ctx.jni_cache);
  if (!minecraft_instance) { return; }

  auto local_player = Minecraft::GetLocalPlayer(env, ctx.jni_cache, minecraft_instance.get());
  auto world = Minecraft::GetWorld(env, ctx.jni_cache, minecraft_instance.get());
  auto game_renderer = Minecraft::GetGameRenderer(env, ctx.jni_cache, minecraft_instance.get());
  if (!local_player || !world || !game_renderer) { return; }

  auto camera = GameRenderer::GetCamera(env, ctx.jni_cache, game_renderer.get());
  if (!camera) { return; }

  const Vec3 camera_position = Camera::GetPosition(env, ctx.jni_cache, camera.get());
  const float current_yaw = Camera::GetYaw(env, ctx.jni_cache, camera.get());
  const float current_pitch = Camera::GetPitch(env, ctx.jni_cache, camera.get());
  const float fov_limit = std::max(fov_degrees_, 0.001f);
  const Vec3 view_direction = ViewDirection(current_yaw, current_pitch);
  constexpr double kMaxPredictedHorizontalSpeed = 2.5;
  const Vec3 local_velocity = ClampHorizontalVelocity(
      Entity::GetVelocity(env, ctx.jni_cache, local_player.get()), kMaxPredictedHorizontalSpeed);
  const double max_distance_sq =
      static_cast<double>(max_distance_) * static_cast<double>(max_distance_);

  bool has_target = false;
  float best_score = 1e30f;
  float best_target_yaw = 0.0f;
  float best_target_pitch = 0.0f;
  int best_target_id = 0;

  for (auto entity : ClientWorld::GetEntities(env, ctx.jni_cache, world.get())) {
    if (!entity) { continue; }
    if (env->IsSameObject(entity.get(), local_player.get()) == JNI_TRUE) { continue; }
    if (!Entity::IsAlive(env, ctx.jni_cache, entity.get()) ||
        Entity::IsInvisible(env, ctx.jni_cache, entity.get())) {
      continue;
    }

    bool matches = false;
    if (target_players_ && ctx.jni_cache.player_entity_class != nullptr &&
        env->IsInstanceOf(entity.get(), ctx.jni_cache.player_entity_class)) {
      matches = true;
    }

    if (!matches && target_hostiles_) {
      const std::string key = Entity::GetTranslationKey(env, ctx.jni_cache, entity.get());
      matches = !key.empty() && IsTrackedHostile(key);
    }

    if (!matches) { continue; }
    if (line_of_sight_only_ &&
        !LivingEntity::HasLineOfSight(env, ctx.jni_cache, local_player.get(), entity.get())) {
      continue;
    }

    auto [entity_x, entity_y, entity_z] = Entity::GetCoordinates(env, ctx.jni_cache, entity.get());
    const double target_eye_y = Entity::GetEyeY(env, ctx.jni_cache, entity.get());
    const double dx = entity_x - camera_position.x;
    const double dy = target_eye_y - camera_position.y;
    const double dz = entity_z - camera_position.z;
    const double distance_sq = dx * dx + dy * dy + dz * dz;
    if (distance_sq <= 0.0001 || distance_sq > max_distance_sq) { continue; }

    const EntityData entity_data = Entity::GetData(env, ctx.jni_cache, entity.get());
    const double half_width = std::max(static_cast<double>(entity_data.width), 0.3) * 0.5;
    const double height = std::max(static_cast<double>(entity_data.height), 0.6);
    const Vec3 target_position{entity_x, entity_y, entity_z};
    const Vec3 target_velocity = ClampHorizontalVelocity(
        Entity::GetVelocity(env, ctx.jni_cache, entity.get()), kMaxPredictedHorizontalSpeed);
    const Vec3 relative_velocity{
        target_velocity.x - local_velocity.x, 0.0, target_velocity.z - local_velocity.z};
    const float prediction_strength = SmartPredictionStrength(std::sqrt(distance_sq),
                                                              static_cast<double>(max_distance_),
                                                              relative_velocity,
                                                              {dx, 0.0, dz});
    const double lead_ticks = PredictionLeadTicks(prediction_strength, smoothness_);
    const Vec3 predicted_target_position =
        PredictPosition(target_position, relative_velocity, lead_ticks);

    const Vec3 current_box_min{entity_x - half_width, entity_y, entity_z - half_width};
    const Vec3 current_box_max{entity_x + half_width, entity_y + height, entity_z + half_width};
    const Vec3 box_min{predicted_target_position.x - half_width,
                       predicted_target_position.y,
                       predicted_target_position.z - half_width};
    const Vec3 box_max{predicted_target_position.x + half_width,
                       predicted_target_position.y + height,
                       predicted_target_position.z + half_width};
    const double aim_y = std::clamp(target_eye_y, box_min.y, box_max.y);
    const Vec3 desired_aim_point{predicted_target_position.x, aim_y, predicted_target_position.z};

    AimPoint scoring_point{};
    if (RayIntersectsAabb(camera_position, view_direction, current_box_min, current_box_max)) {
      scoring_point = BuildAimPoint(camera_position, current_yaw, current_pitch, desired_aim_point);
      scoring_point.fov_delta = 0.0f;
    } else {
      const std::array<double, 3> xs = {box_min.x, predicted_target_position.x, box_max.x};
      const std::array<double, 5> ys = {
          box_min.y, box_min.y + height * 0.35, box_min.y + height * 0.55, aim_y, box_max.y};
      const std::array<double, 3> zs = {box_min.z, predicted_target_position.z, box_max.z};

      for (const double sample_x : xs) {
        for (const double sample_y : ys) {
          for (const double sample_z : zs) {
            const AimPoint sample = BuildAimPoint(
                camera_position, current_yaw, current_pitch, {sample_x, sample_y, sample_z});
            if (sample.fov_delta < scoring_point.fov_delta) { scoring_point = sample; }
          }
        }
      }
    }

    const float fov_delta = scoring_point.fov_delta;
    if (fov_delta > fov_limit) { continue; }

    const AimPoint aim_point =
        BuildAimPoint(camera_position, current_yaw, current_pitch, desired_aim_point);
    if (aim_point.distance_sq <= 0.0001) { continue; }

    const float normalized_fov = std::clamp(fov_delta / fov_limit, 0.0f, 1.0f);
    const float normalized_distance =
        std::clamp(static_cast<float>(distance_sq / max_distance_sq), 0.0f, 1.0f);
    float score = normalized_fov * 0.75f + normalized_distance * 0.25f;
    const int eid = Entity::GetId(env, ctx.jni_cache, entity.get());
    if (eid != 0 && eid == locked_target_id_) { score *= 0.82f; }

    if (score < best_score) {
      best_score = score;
      best_target_yaw = aim_point.yaw;
      best_target_pitch = aim_point.pitch;
      best_target_id = eid;
      has_target = true;
    }
  }

  if (!has_target) {
    locked_target_id_ = 0;
    return;
  }

  locked_target_id_ = best_target_id;

  const float next_yaw = current_yaw + WrapDegrees(best_target_yaw - current_yaw) / smoothness_;
  const float next_pitch = current_pitch + (best_target_pitch - current_pitch) / smoothness_;

  Entity::SetYaw(env, ctx.jni_cache, local_player.get(), next_yaw);
  Entity::SetPitch(env, ctx.jni_cache, local_player.get(), ClampPitch(next_pitch));
}

}  // namespace mc_internal
