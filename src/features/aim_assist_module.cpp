#include "mc_internal/features/aim_assist_module.hpp"
#include "mc_internal/features/aim_assist_detail.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <limits>
#include <string>
#include <unordered_set>

#include "imgui.h"

#include "mc_internal/context.hpp"
#include "mc_internal/sdk/minecraft.hpp"
#include "mc_internal/ui/widgets.hpp"

namespace mc_internal {

namespace {

const std::unordered_set<std::string_view> kHostileKeys = {
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

struct TargetCandidate {
  bool valid = false;
  int entity_id = 0;
  float score = std::numeric_limits<float>::max();
  float yaw = 0.0f;
  float pitch = 0.0f;
};

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

void AimAssistModule::on_disable() {
  clear_locked_target();
  activation_was_down_ = false;
  waiting_for_bind_ = false;
  bind_capture_armed_ = false;
  runtime_status_ = RuntimeStatus::kIdle;
  hostile_key_status_ = HostileKeyStatus::kUnknown;
}

void AimAssistModule::clear_locked_target() noexcept {
  has_locked_target_ = false;
  locked_target_id_ = 0;
}

void AimAssistModule::reset_runtime_state(RuntimeStatus status, bool activation_was_down) noexcept {
  clear_locked_target();
  runtime_status_ = status;
  activation_was_down_ = activation_was_down;
  if (!target_hostiles_) { hostile_key_status_ = HostileKeyStatus::kUnknown; }
}

void AimAssistModule::set_tracking_state(int target_id) noexcept {
  has_locked_target_ = true;
  locked_target_id_ = target_id;
  runtime_status_ = RuntimeStatus::kTracking;
}

const char* AimAssistModule::runtime_status_label() const noexcept {
  switch (runtime_status_) {
    case RuntimeStatus::kIdle:
      return "idle";
    case RuntimeStatus::kBindUnbound:
      return "activation bind unbound";
    case RuntimeStatus::kNoTargetTypesEnabled:
      return "no target types enabled";
    case RuntimeStatus::kInvalidConfig:
      return "invalid runtime config";
    case RuntimeStatus::kJvmUnavailable:
      return "jni unavailable";
    case RuntimeStatus::kCacheUnavailable:
      return "jni cache unavailable";
    case RuntimeStatus::kNotInGame:
      return "not in game";
    case RuntimeStatus::kScreenOpen:
      return "menu or screen open";
    case RuntimeStatus::kCameraUnavailable:
      return "camera unavailable";
    case RuntimeStatus::kInvalidRuntimeData:
      return "invalid runtime data";
    case RuntimeStatus::kHostileMappingsUnavailable:
      return "hostile translation metadata unavailable";
    case RuntimeStatus::kNoTarget:
      return "no valid target";
    case RuntimeStatus::kTracking:
      return "tracking target";
  }
  return "unknown";
}

const char* AimAssistModule::hostile_key_status_label() const noexcept {
  switch (hostile_key_status_) {
    case HostileKeyStatus::kUnknown:
      return nullptr;
    case HostileKeyStatus::kReady:
      return "hostile keys ready";
    case HostileKeyStatus::kUnavailable:
      return "hostile keys unavailable for this mapping set";
  }
  return nullptr;
}

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
    ImGui::Separator();
    ImGui::Text("status: %s", runtime_status_label());
    if (has_locked_target_) { ImGui::Text("locked target id: %d", locked_target_id_); }
    if (target_hostiles_) {
      if (const char* hostile_key_status = hostile_key_status_label();
          hostile_key_status != nullptr) {
        ImGui::Text("hostiles: %s", hostile_key_status);
      }
    }
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

  ImGui::Spacing();
  ImGui::TextDisabled("status: %s", runtime_status_label());
  if (has_locked_target_) { ImGui::TextDisabled("locked target id: %d", locked_target_id_); }
  if (target_hostiles_) {
    if (const char* hostile_key_status = hostile_key_status_label();
        hostile_key_status != nullptr) {
      ImGui::TextDisabled("hostiles: %s", hostile_key_status);
    }
  }

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
  if (!target_hostiles_) { hostile_key_status_ = HostileKeyStatus::kUnknown; }

  if (activation_bind_code_ < 0) {
    reset_runtime_state(RuntimeStatus::kBindUnbound, false);
    return;
  }

  const bool activation_down = is_activation_held(ctx);
  if (!activation_down) {
    clear_locked_target();
    activation_was_down_ = false;
    runtime_status_ = RuntimeStatus::kIdle;
    return;
  }

  if (!target_players_ && !target_hostiles_) {
    reset_runtime_state(RuntimeStatus::kNoTargetTypesEnabled, activation_down);
    return;
  }
  if (!aim_assist_detail::IsRuntimeConfigValid(fov_degrees_, smoothness_, max_distance_)) {
    reset_runtime_state(RuntimeStatus::kInvalidConfig, activation_down);
    return;
  }
  if (ctx.show_menu) {
    reset_runtime_state(RuntimeStatus::kScreenOpen, activation_down);
    return;
  }
  if (!ctx.jvm_attachment) {
    reset_runtime_state(RuntimeStatus::kJvmUnavailable, activation_down);
    return;
  }

  const JniEnv env(ctx.jvm_attachment->env());
  if (!ctx.jni_cache.Initialize(env)) {
    reset_runtime_state(RuntimeStatus::kCacheUnavailable, activation_down);
    return;
  }

  const bool hostile_keys_ready = aim_assist_detail::CanResolveHostileTargetKeys(
      target_hostiles_,
      ctx.jni_cache.entity_get_type != nullptr,
      ctx.jni_cache.entity_type_translation_key_field != nullptr);
  hostile_key_status_ = !target_hostiles_ ? HostileKeyStatus::kUnknown
                                          : (hostile_keys_ready ? HostileKeyStatus::kReady
                                                                : HostileKeyStatus::kUnavailable);
  if (!hostile_keys_ready && !target_players_) {
    reset_runtime_state(RuntimeStatus::kHostileMappingsUnavailable, activation_down);
    return;
  }

  auto minecraft_instance = Minecraft::GetInstance(env, ctx.jni_cache);
  if (!minecraft_instance) {
    reset_runtime_state(RuntimeStatus::kNotInGame, activation_down);
    return;
  }
  if (Minecraft::HasOpenScreen(env, ctx.jni_cache, minecraft_instance.get())) {
    reset_runtime_state(RuntimeStatus::kScreenOpen, activation_down);
    return;
  }

  auto local_player = Minecraft::GetLocalPlayer(env, ctx.jni_cache, minecraft_instance.get());
  auto world = Minecraft::GetWorld(env, ctx.jni_cache, minecraft_instance.get());
  auto game_renderer = Minecraft::GetGameRenderer(env, ctx.jni_cache, minecraft_instance.get());
  if (!local_player || !world || !game_renderer) {
    reset_runtime_state(RuntimeStatus::kNotInGame, activation_down);
    return;
  }

  auto camera = GameRenderer::GetCamera(env, ctx.jni_cache, game_renderer.get());
  if (!camera) {
    reset_runtime_state(RuntimeStatus::kCameraUnavailable, activation_down);
    return;
  }

  const Vec3 camera_position = Camera::GetPosition(env, ctx.jni_cache, camera.get());
  const float current_yaw = Camera::GetYaw(env, ctx.jni_cache, camera.get());
  const float current_pitch = Camera::GetPitch(env, ctx.jni_cache, camera.get());
  if (!aim_assist_detail::IsFinite(camera_position) || !aim_assist_detail::IsFinite(current_yaw) ||
      !aim_assist_detail::IsFinite(current_pitch)) {
    reset_runtime_state(RuntimeStatus::kInvalidRuntimeData, activation_down);
    return;
  }

  const float fov_limit = std::max(fov_degrees_, 0.001f);
  const Vec3 view_direction = aim_assist_detail::ViewDirection(current_yaw, current_pitch);
  constexpr double kMaxPredictedHorizontalSpeed = 2.5;
  const Vec3 local_velocity = aim_assist_detail::ClampHorizontalVelocity(
      Entity::GetVelocity(env, ctx.jni_cache, local_player.get()), kMaxPredictedHorizontalSpeed);
  if (!aim_assist_detail::IsFinite(view_direction) ||
      !aim_assist_detail::IsFinite(local_velocity)) {
    reset_runtime_state(RuntimeStatus::kInvalidRuntimeData, activation_down);
    return;
  }

  const double max_distance_sq =
      static_cast<double>(max_distance_) * static_cast<double>(max_distance_);
  const auto search_plan = aim_assist_detail::PlanTargetSearch(
      activation_was_down_, has_locked_target_, locked_target_id_);

  const auto build_candidate =
      [&](jobject entity, bool prefer_locked_target, int requested_entity_id) -> TargetCandidate {
    JniLocalFrame local_frame(env.get(), 24);
    if (!local_frame || entity == nullptr) { return {}; }
    if (env->IsSameObject(entity, local_player.get()) == JNI_TRUE) { return {}; }
    if (!Entity::IsAlive(env, ctx.jni_cache, entity) ||
        Entity::IsInvisible(env, ctx.jni_cache, entity)) {
      return {};
    }

    const int eid = Entity::GetId(env, ctx.jni_cache, entity);
    if (prefer_locked_target && eid != requested_entity_id) { return {}; }

    bool matches = false;
    if (target_players_ && ctx.jni_cache.player_entity_class != nullptr &&
        env->IsInstanceOf(entity, ctx.jni_cache.player_entity_class)) {
      matches = true;
    }

    if (!matches && target_hostiles_ && hostile_keys_ready) {
      matches = Entity::IsTranslationKeyInSet(env, ctx.jni_cache, entity, kHostileKeys);
    }

    if (!matches) { return {}; }
    if (line_of_sight_only_ &&
        !LivingEntity::HasLineOfSight(env, ctx.jni_cache, local_player.get(), entity)) {
      return {};
    }

    auto [entity_x, entity_y, entity_z] = Entity::GetCoordinates(env, ctx.jni_cache, entity);
    const double target_eye_y = Entity::GetEyeY(env, ctx.jni_cache, entity);
    const Vec3 target_position{entity_x, entity_y, entity_z};
    if (!aim_assist_detail::IsFinite(target_position) ||
        !aim_assist_detail::IsFinite(target_eye_y)) {
      return {};
    }

    const double dx = entity_x - camera_position.x;
    const double dy = target_eye_y - camera_position.y;
    const double dz = entity_z - camera_position.z;
    const double distance_sq = dx * dx + dy * dy + dz * dz;
    if (!aim_assist_detail::IsFinite(distance_sq) || distance_sq <= 0.0001 ||
        distance_sq > max_distance_sq) {
      return {};
    }

    const EntityData entity_data = Entity::GetData(env, ctx.jni_cache, entity);
    if (!aim_assist_detail::IsFinite(entity_data.width) ||
        !aim_assist_detail::IsFinite(entity_data.height)) {
      return {};
    }

    const double half_width = std::max(static_cast<double>(entity_data.width), 0.3) * 0.5;
    const double height = std::max(static_cast<double>(entity_data.height), 0.6);
    if (!aim_assist_detail::IsFinite(half_width) || !aim_assist_detail::IsFinite(height)) {
      return {};
    }

    const Vec3 target_velocity = aim_assist_detail::ClampHorizontalVelocity(
        Entity::GetVelocity(env, ctx.jni_cache, entity), kMaxPredictedHorizontalSpeed);
    const Vec3 relative_velocity{
        target_velocity.x - local_velocity.x, 0.0, target_velocity.z - local_velocity.z};
    if (!aim_assist_detail::IsFinite(target_velocity) ||
        !aim_assist_detail::IsFinite(relative_velocity)) {
      return {};
    }

    const float prediction_strength =
        aim_assist_detail::SmartPredictionStrength(std::sqrt(distance_sq),
                                                   static_cast<double>(max_distance_),
                                                   relative_velocity,
                                                   {dx, 0.0, dz});
    const double lead_ticks = aim_assist_detail::PredictionLeadTicks(
        prediction_strength, smoothness_, ImGui::GetIO().DeltaTime);
    if (!aim_assist_detail::IsFinite(prediction_strength) ||
        !aim_assist_detail::IsFinite(lead_ticks)) {
      return {};
    }

    const Vec3 predicted_target_position =
        aim_assist_detail::PredictPosition(target_position, relative_velocity, lead_ticks);
    if (!aim_assist_detail::IsFinite(predicted_target_position)) { return {}; }

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
    const aim_assist_detail::AimPoint aim_point = aim_assist_detail::BuildAimPoint(
        camera_position, current_yaw, current_pitch, desired_aim_point);
    if (!aim_assist_detail::PassesHardFov(aim_point, fov_limit)) { return {}; }

    aim_assist_detail::AimPoint scoring_point{};
    if (aim_assist_detail::RayIntersectsAabb(
            camera_position, view_direction, current_box_min, current_box_max)) {
      scoring_point = aim_point;
      scoring_point.fov_delta = 0.0f;
    } else {
      const std::array<double, 3> xs = {box_min.x, predicted_target_position.x, box_max.x};
      const std::array<double, 5> ys = {
          box_min.y, box_min.y + height * 0.35, box_min.y + height * 0.55, aim_y, box_max.y};
      const std::array<double, 3> zs = {box_min.z, predicted_target_position.z, box_max.z};

      for (const double sample_x : xs) {
        for (const double sample_y : ys) {
          for (const double sample_z : zs) {
            const aim_assist_detail::AimPoint sample = aim_assist_detail::BuildAimPoint(
                camera_position, current_yaw, current_pitch, {sample_x, sample_y, sample_z});
            if (aim_assist_detail::IsAimPointValid(sample) &&
                sample.fov_delta < scoring_point.fov_delta) {
              scoring_point = sample;
            }
            if (!aim_assist_detail::ShouldContinueFovSampling(scoring_point.fov_delta)) { break; }
          }
          if (!aim_assist_detail::ShouldContinueFovSampling(scoring_point.fov_delta)) { break; }
        }
        if (!aim_assist_detail::ShouldContinueFovSampling(scoring_point.fov_delta)) { break; }
      }
    }

    if (!aim_assist_detail::IsAimPointValid(scoring_point)) { return {}; }

    const float normalized_fov = std::clamp(scoring_point.fov_delta / fov_limit, 0.0f, 1.0f);
    const float normalized_distance =
        std::clamp(static_cast<float>(distance_sq / max_distance_sq), 0.0f, 1.0f);
    const float score = normalized_fov * 0.75f + normalized_distance * 0.25f;
    if (!aim_assist_detail::IsFinite(score)) { return {}; }

    return {true, eid, score, aim_point.yaw, aim_point.pitch};
  };

  const bool has_locked_target_fast_path = aim_assist_detail::CanUseLockedTargetFastPath(
      search_plan, ctx.jni_cache.client_world_get_entity_by_id != nullptr);

  const auto find_best_target = [&](bool prefer_locked_target, int requested_entity_id) {
    if (prefer_locked_target && has_locked_target_fast_path) {
      auto entity =
          ClientWorld::GetEntityById(env, ctx.jni_cache, world.get(), requested_entity_id);
      return build_candidate(entity.get(), true, requested_entity_id);
    }

    TargetCandidate best_target{};
    for (auto entity : ClientWorld::GetEntities(env, ctx.jni_cache, world.get())) {
      TargetCandidate candidate =
          build_candidate(entity.get(), prefer_locked_target, requested_entity_id);
      if (!candidate.valid) { continue; }

      if (candidate.score < best_target.score) {
        best_target = candidate;
        if (prefer_locked_target) { break; }
      }
    }
    return best_target;
  };

  TargetCandidate best_target =
      find_best_target(search_plan.prefer_locked_target, search_plan.requested_target_id);
  if (!best_target.valid && search_plan.retry_without_lock) {
    best_target = find_best_target(false, 0);
  }

  if (!best_target.valid) {
    reset_runtime_state(RuntimeStatus::kNoTarget, activation_down);
    return;
  }

  activation_was_down_ = activation_down;

  const float next_yaw =
      current_yaw + aim_assist_detail::WrapDegrees(best_target.yaw - current_yaw) / smoothness_;
  const float next_pitch = current_pitch + (best_target.pitch - current_pitch) / smoothness_;
  if (!aim_assist_detail::IsFinite(next_yaw) || !aim_assist_detail::IsFinite(next_pitch)) {
    reset_runtime_state(RuntimeStatus::kInvalidRuntimeData, activation_down);
    return;
  }

  Entity::SetYaw(env, ctx.jni_cache, local_player.get(), next_yaw);
  Entity::SetPitch(
      env, ctx.jni_cache, local_player.get(), aim_assist_detail::ClampPitch(next_pitch));
  set_tracking_state(best_target.entity_id);
}

}  // namespace mc_internal
