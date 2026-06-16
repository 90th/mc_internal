#include "mc_internal/features/aim_assist_module.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <limits>
#include <string>

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

struct TargetCandidate {
  bool valid = false;
  int entity_id = 0;
  float score = std::numeric_limits<float>::max();
  float yaw = 0.0f;
  float pitch = 0.0f;
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

std::string MouseButtonName(int button) {
  switch (button) {
    case GLFW_MOUSE_BUTTON_LEFT:
      return "left mouse";
    case GLFW_MOUSE_BUTTON_RIGHT:
      return "right mouse";
    case GLFW_MOUSE_BUTTON_MIDDLE:
      return "middle mouse";
    default:
      return "mouse " + std::to_string(button + 1);
  }
}

std::string KeyName(int key) {
  if (key >= GLFW_KEY_A && key <= GLFW_KEY_Z) {
    return std::string(1, static_cast<char>('a' + key - GLFW_KEY_A));
  }
  if (key >= GLFW_KEY_0 && key <= GLFW_KEY_9) {
    return std::string(1, static_cast<char>('0' + key - GLFW_KEY_0));
  }
  if (key >= GLFW_KEY_F1 && key <= GLFW_KEY_F12) {
    return "f" + std::to_string(key - GLFW_KEY_F1 + 1);
  }

  switch (key) {
    case GLFW_KEY_SPACE:
      return "space";
    case GLFW_KEY_TAB:
      return "tab";
    case GLFW_KEY_ENTER:
      return "enter";
    case GLFW_KEY_BACKSPACE:
      return "backspace";
    case GLFW_KEY_ESCAPE:
      return "escape";
    case GLFW_KEY_LEFT_SHIFT:
      return "left shift";
    case GLFW_KEY_RIGHT_SHIFT:
      return "right shift";
    case GLFW_KEY_LEFT_CONTROL:
      return "left control";
    case GLFW_KEY_RIGHT_CONTROL:
      return "right control";
    case GLFW_KEY_LEFT_ALT:
      return "left alt";
    case GLFW_KEY_RIGHT_ALT:
      return "right alt";
    case GLFW_KEY_INSERT:
      return "insert";
    case GLFW_KEY_DELETE:
      return "delete";
    case GLFW_KEY_HOME:
      return "home";
    case GLFW_KEY_END:
      return "end";
    case GLFW_KEY_PAGE_UP:
      return "page up";
    case GLFW_KEY_PAGE_DOWN:
      return "page down";
    default:
      return "key " + std::to_string(key);
  }
}

bool AnyBindInputDown(const OverlayContext& ctx) {
  if (ctx.pinned_window == nullptr) { return false; }

  if (ctx.glfw.get_mouse_button != nullptr) {
    for (int button = GLFW_MOUSE_BUTTON_1; button <= GLFW_MOUSE_BUTTON_LAST; ++button) {
      if (ctx.glfw.get_mouse_button(ctx.pinned_window, button) == GLFW_PRESS) { return true; }
    }
  }

  if (ctx.glfw.get_key != nullptr) {
    for (int key = GLFW_KEY_SPACE; key <= GLFW_KEY_LAST; ++key) {
      if (ctx.glfw.get_key(ctx.pinned_window, key) == GLFW_PRESS) { return true; }
    }
  }

  return false;
}

}  // namespace

AimAssistModule::AimAssistModule()
    : Module("aim assist",
             "smoothly nudges aim toward nearby targets while attacking",
             ModuleCategory::kCombat) {}

bool AimAssistModule::is_activation_held(const OverlayContext& ctx) const {
  if (ctx.pinned_window == nullptr || activation_bind_code_ < 0) { return false; }

  if (activation_bind_type_ == ActivationBindType::kMouse) {
    return ctx.glfw.get_mouse_button != nullptr &&
           ctx.glfw.get_mouse_button(ctx.pinned_window, activation_bind_code_) == GLFW_PRESS;
  }

  return ctx.glfw.get_key != nullptr &&
         ctx.glfw.get_key(ctx.pinned_window, activation_bind_code_) == GLFW_PRESS;
}

std::string AimAssistModule::activation_bind_label() const {
  if (activation_bind_code_ < 0) { return "unbound"; }
  if (activation_bind_type_ == ActivationBindType::kMouse) {
    return MouseButtonName(activation_bind_code_);
  }
  return KeyName(activation_bind_code_);
}

void AimAssistModule::update_bind_capture(const OverlayContext& ctx) {
  if (!waiting_for_bind_ || ctx.pinned_window == nullptr) { return; }

  if (!bind_capture_armed_) {
    bind_capture_armed_ = !AnyBindInputDown(ctx);
    return;
  }

  if (ctx.glfw.get_key != nullptr) {
    if (ctx.glfw.get_key(ctx.pinned_window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
      waiting_for_bind_ = false;
      bind_capture_armed_ = false;
      return;
    }
    if (ctx.glfw.get_key(ctx.pinned_window, GLFW_KEY_BACKSPACE) == GLFW_PRESS) {
      activation_bind_code_ = -1;
      waiting_for_bind_ = false;
      bind_capture_armed_ = false;
      return;
    }
  }

  if (ctx.glfw.get_mouse_button != nullptr) {
    for (int button = GLFW_MOUSE_BUTTON_1; button <= GLFW_MOUSE_BUTTON_LAST; ++button) {
      if (ctx.glfw.get_mouse_button(ctx.pinned_window, button) == GLFW_PRESS) {
        activation_bind_type_ = ActivationBindType::kMouse;
        activation_bind_code_ = button;
        waiting_for_bind_ = false;
        bind_capture_armed_ = false;
        return;
      }
    }
  }

  if (ctx.glfw.get_key != nullptr) {
    for (int key = GLFW_KEY_SPACE; key <= GLFW_KEY_LAST; ++key) {
      if (ctx.glfw.get_key(ctx.pinned_window, key) == GLFW_PRESS) {
        activation_bind_type_ = ActivationBindType::kKeyboard;
        activation_bind_code_ = key;
        waiting_for_bind_ = false;
        bind_capture_armed_ = false;
        return;
      }
    }
  }
}

void AimAssistModule::on_render_header_controls(const OverlayContext& ctx) {
  const std::string bind_label = waiting_for_bind_ ? "press key..." : activation_bind_label();
  char button_label[96];
  std::snprintf(button_label, sizeof(button_label), "%s##aim_activation_bind", bind_label.c_str());

  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 3.0f));
  if (ImGui::Button(button_label)) {
    waiting_for_bind_ = true;
    bind_capture_armed_ = false;
  }
  ImGui::PopStyleVar();

  if (ImGui::IsItemHovered()) {
    ImGui::BeginTooltip();
    ImGui::TextUnformatted("hold this bind to select and track a target");
    ImGui::TextUnformatted("escape cancels, backspace clears while recording");
    ImGui::EndTooltip();
  }

  if (waiting_for_bind_) { update_bind_capture(ctx); }
}

void AimAssistModule::on_render_settings(const OverlayContext& ctx) {
  static_cast<void>(ctx);

  ui::SectionHeader(
      "targeting",
      "nudges your crosshair toward the closest target inside the configured field of view.");

  ui::Toggle("target players", &target_players_);
  ImGui::SameLine(0.0f, 14.0f);
  ui::Toggle("target hostiles", &target_hostiles_);
  ImGui::SameLine(0.0f, 14.0f);
  ui::Toggle("line of sight only", &line_of_sight_only_);

  ui::SectionHeader(
      "tuning",
      "higher smoothness slows the correction and keeps the movement looking more human. target "
      "choice blends crosshair error with distance. prediction automatically adapts to movement.");

  ui::LabeledSlider("field of view", "##aim_assist_fov", &fov_degrees_, 1.0f, 180.0f, "%.0f deg");
  ui::LabeledSlider("smoothness", "##aim_assist_smooth", &smoothness_, 1.0f, 20.0f, "%.1f");
  ui::LabeledSlider(
      "max distance", "##aim_assist_distance", &max_distance_, 8.0f, 192.0f, "%.0f blocks");
}

void AimAssistModule::on_render_3d(const OverlayContext& ctx) {
  const bool activation_down = is_activation_held(ctx);
  if (!activation_down) {
    locked_target_id_ = 0;
    activation_was_down_ = false;
    return;
  }

  if ((!target_players_ && !target_hostiles_) || ctx.show_menu || !ctx.jvm_attachment) {
    locked_target_id_ = 0;
    activation_was_down_ = activation_down;
    return;
  }

  const JniEnv env(ctx.jvm_attachment->env());
  if (!ctx.jni_cache.Initialize(env)) {
    locked_target_id_ = 0;
    activation_was_down_ = activation_down;
    return;
  }

  auto minecraft_instance = Minecraft::GetInstance(env, ctx.jni_cache);
  if (!minecraft_instance ||
      Minecraft::HasOpenScreen(env, ctx.jni_cache, minecraft_instance.get())) {
    locked_target_id_ = 0;
    activation_was_down_ = activation_down;
    return;
  }

  auto local_player = Minecraft::GetLocalPlayer(env, ctx.jni_cache, minecraft_instance.get());
  auto world = Minecraft::GetWorld(env, ctx.jni_cache, minecraft_instance.get());
  auto game_renderer = Minecraft::GetGameRenderer(env, ctx.jni_cache, minecraft_instance.get());
  if (!local_player || !world || !game_renderer) {
    locked_target_id_ = 0;
    activation_was_down_ = activation_down;
    return;
  }

  auto camera = GameRenderer::GetCamera(env, ctx.jni_cache, game_renderer.get());
  if (!camera) {
    locked_target_id_ = 0;
    activation_was_down_ = activation_down;
    return;
  }

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

  const bool select_new_target = !activation_was_down_;
  const int required_target_id = select_new_target ? 0 : locked_target_id_;
  if (!select_new_target && required_target_id == 0) {
    activation_was_down_ = activation_down;
    return;
  }

  const auto build_candidate = [&](jobject entity, int requested_entity_id) -> TargetCandidate {
    if (entity == nullptr) { return {}; }
    if (env->IsSameObject(entity, local_player.get()) == JNI_TRUE) { return {}; }
    if (!Entity::IsAlive(env, ctx.jni_cache, entity) ||
        Entity::IsInvisible(env, ctx.jni_cache, entity)) {
      return {};
    }

    const int eid = Entity::GetId(env, ctx.jni_cache, entity);
    if (eid == 0 || (requested_entity_id != 0 && eid != requested_entity_id)) { return {}; }

    bool matches = false;
    if (target_players_ && ctx.jni_cache.player_entity_class != nullptr &&
        env->IsInstanceOf(entity, ctx.jni_cache.player_entity_class)) {
      matches = true;
    }

    if (!matches && target_hostiles_) {
      const std::string key = Entity::GetTranslationKey(env, ctx.jni_cache, entity);
      matches = !key.empty() && IsTrackedHostile(key);
    }

    if (!matches) { return {}; }
    if (line_of_sight_only_ &&
        !LivingEntity::HasLineOfSight(env, ctx.jni_cache, local_player.get(), entity)) {
      return {};
    }

    auto [entity_x, entity_y, entity_z] = Entity::GetCoordinates(env, ctx.jni_cache, entity);
    const double target_eye_y = Entity::GetEyeY(env, ctx.jni_cache, entity);
    const double dx = entity_x - camera_position.x;
    const double dy = target_eye_y - camera_position.y;
    const double dz = entity_z - camera_position.z;
    const double distance_sq = dx * dx + dy * dy + dz * dz;
    if (distance_sq <= 0.0001 || distance_sq > max_distance_sq) { return {}; }

    const EntityData entity_data = Entity::GetData(env, ctx.jni_cache, entity);
    const double half_width = std::max(static_cast<double>(entity_data.width), 0.3) * 0.5;
    const double height = std::max(static_cast<double>(entity_data.height), 0.6);
    const Vec3 target_position{entity_x, entity_y, entity_z};
    const Vec3 target_velocity = ClampHorizontalVelocity(
        Entity::GetVelocity(env, ctx.jni_cache, entity), kMaxPredictedHorizontalSpeed);
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
    if (fov_delta > fov_limit) { return {}; }

    const AimPoint aim_point =
        BuildAimPoint(camera_position, current_yaw, current_pitch, desired_aim_point);
    if (aim_point.distance_sq <= 0.0001) { return {}; }

    const float normalized_fov = std::clamp(fov_delta / fov_limit, 0.0f, 1.0f);
    const float normalized_distance =
        std::clamp(static_cast<float>(distance_sq / max_distance_sq), 0.0f, 1.0f);
    const float score = normalized_fov * 0.75f + normalized_distance * 0.25f;
    return {true, eid, score, aim_point.yaw, aim_point.pitch};
  };

  TargetCandidate best_target{};
  for (auto entity : ClientWorld::GetEntities(env, ctx.jni_cache, world.get())) {
    TargetCandidate candidate = build_candidate(entity.get(), required_target_id);
    if (!candidate.valid) { continue; }

    if (candidate.score < best_target.score) {
      best_target = candidate;
      if (required_target_id != 0) { break; }
    }
  }

  if (!best_target.valid) {
    locked_target_id_ = 0;
    activation_was_down_ = activation_down;
    return;
  }

  locked_target_id_ = best_target.entity_id;
  activation_was_down_ = activation_down;

  const float next_yaw = current_yaw + WrapDegrees(best_target.yaw - current_yaw) / smoothness_;
  const float next_pitch = current_pitch + (best_target.pitch - current_pitch) / smoothness_;

  Entity::SetYaw(env, ctx.jni_cache, local_player.get(), next_yaw);
  Entity::SetPitch(env, ctx.jni_cache, local_player.get(), ClampPitch(next_pitch));
}

}  // namespace mc_internal
