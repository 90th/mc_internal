#include "mc_internal/features/esp_module.hpp"

#include <algorithm>
#include <array>
#include <cmath>

#include "imgui.h"

#include "mc_internal/context.hpp"
#include "mc_internal/core/jvm_attachment.hpp"
#include "mc_internal/sdk/math.hpp"
#include "mc_internal/sdk/minecraft.hpp"
#include "mc_internal/ui/widgets.hpp"

namespace mc_internal {

namespace {

constexpr float kPi = 3.14159265358979323846f;
constexpr float kMinRenderDistance = 16.0f;
constexpr float kMaxRenderDistance = 512.0f;
constexpr ImGuiColorEditFlags kGroupColorEditFlags =
    ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_AlphaPreview |
    ImGuiColorEditFlags_NoTooltip;

float ToRadians(float degrees) { return degrees * (kPi / 180.0f); }

double Lerp(double start, double end, float delta) { return start + (end - start) * delta; }

double Dot(const Vec3& a, const Vec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

Vec3 Cross(const Vec3& a, const Vec3& b) {
  return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

Vec3 Normalize(const Vec3& vector) {
  const double length = std::sqrt(Dot(vector, vector));
  if (length <= 0.000001) { return {}; }
  return {vector.x / length, vector.y / length, vector.z / length};
}

void SetIdentity(float* matrix) {
  for (int index = 0; index < 16; ++index) { matrix[index] = (index % 5 == 0) ? 1.0f : 0.0f; }
}

void BuildPerspectiveMatrix(
    float fov_degrees, float aspect_ratio, float near_plane, float far_plane, float* matrix) {
  SetIdentity(matrix);

  const float focal_length = 1.0f / std::tan(ToRadians(fov_degrees) * 0.5f);
  matrix[0] = focal_length / aspect_ratio;
  matrix[5] = focal_length;
  matrix[10] = (far_plane + near_plane) / (near_plane - far_plane);
  matrix[11] = -1.0f;
  matrix[14] = (2.0f * far_plane * near_plane) / (near_plane - far_plane);
  matrix[15] = 0.0f;
}

void BuildViewMatrix(const Vec3& camera_position,
                     float pitch_degrees,
                     float yaw_degrees,
                     float* matrix) {
  const float pitch = ToRadians(pitch_degrees);
  const float yaw = ToRadians(yaw_degrees);

  const Vec3 forward = Normalize(
      {-std::sin(yaw) * std::cos(pitch), -std::sin(pitch), std::cos(yaw) * std::cos(pitch)});

  Vec3 right;
  if (std::abs(pitch_degrees) > 89.0f) {
    right = {-std::cos(yaw), 0.0, -std::sin(yaw)};
  } else {
    const Vec3 world_up = {0.0, 1.0, 0.0};
    right = Normalize(Cross(forward, world_up));
  }

  const Vec3 up = Normalize(Cross(right, forward));

  SetIdentity(matrix);
  matrix[0] = static_cast<float>(right.x);
  matrix[4] = static_cast<float>(right.y);
  matrix[8] = static_cast<float>(right.z);
  matrix[12] = static_cast<float>(-Dot(right, camera_position));

  matrix[1] = static_cast<float>(up.x);
  matrix[5] = static_cast<float>(up.y);
  matrix[9] = static_cast<float>(up.z);
  matrix[13] = static_cast<float>(-Dot(up, camera_position));

  matrix[2] = static_cast<float>(-forward.x);
  matrix[6] = static_cast<float>(-forward.y);
  matrix[10] = static_cast<float>(-forward.z);
  matrix[14] = static_cast<float>(Dot(forward, camera_position));
}

ImU32 ToImColor(const std::array<float, 4>& color) {
  return ImGui::ColorConvertFloat4ToU32(ImVec4(color[0], color[1], color[2], color[3]));
}

void RenderTargetGroupRow(const char* label,
                          const char* color_id,
                          bool* enabled,
                          std::array<float, 4>* color) {
  const float color_button_size = ImGui::GetFrameHeight();

  ImGui::Checkbox(label, enabled);

  ImGui::SameLine();
  const float current_x = ImGui::GetCursorPosX();
  const float color_x = current_x + ImGui::GetContentRegionAvail().x - color_button_size;
  ImGui::SetCursorPosX(std::max(current_x, color_x));
  ImGui::ColorEdit4(color_id, color->data(), kGroupColorEditFlags);
}

}  // namespace

EspModule::EspModule()
    : Module("entity esp",
             "draws projected 2d boxes around world entities",
             ModuleCategory::kVisuals) {}

void EspModule::on_render_settings(const OverlayContext& ctx) {
  static_cast<void>(ctx);

  ui::SectionHeader("target groups");

  RenderTargetGroupRow(
      "players", "##esp_players_color", &player_group_.enabled, &player_group_.color);
  RenderTargetGroupRow(
      "hostiles", "##esp_hostiles_color", &hostile_group_.enabled, &hostile_group_.color);
  RenderTargetGroupRow(
      "passives", "##esp_passives_color", &passive_group_.enabled, &passive_group_.color);
  RenderTargetGroupRow("items", "##esp_items_color", &item_group_.enabled, &item_group_.color);

  ui::SectionHeader("render distance");

  ImGui::PushItemWidth(-1.0f);
  ui::SliderFloat("##esp_max_render_distance",
                  &max_render_distance_,
                  kMinRenderDistance,
                  kMaxRenderDistance,
                  "%.0f blocks");
  ImGui::PopItemWidth();

  ImGui::Spacing();
}

void EspModule::on_render_3d(const OverlayContext& ctx) {
  if (!player_group_.enabled && !hostile_group_.enabled && !passive_group_.enabled &&
      !item_group_.enabled) {
    return;
  }
  if (ctx.display_width <= 0 || ctx.display_height <= 0) { return; }

  if (!ctx.jvm_attachment) { return; }

  const JniEnv env(ctx.jvm_attachment->env());
  if (!ctx.jni_cache.Initialize(env)) { return; }

  auto minecraft_instance = Minecraft::GetInstance(env, ctx.jni_cache);
  if (!minecraft_instance) { return; }

  auto world = Minecraft::GetWorld(env, ctx.jni_cache, minecraft_instance.get());
  if (!world) { return; }

  auto game_renderer = Minecraft::GetGameRenderer(env, ctx.jni_cache, minecraft_instance.get());
  if (!game_renderer) { return; }

  const float tick_delta = Minecraft::GetTickDelta(env, ctx.jni_cache, minecraft_instance.get());
  auto camera = GameRenderer::GetCamera(env, ctx.jni_cache, game_renderer.get());
  if (!camera) { return; }

  const Vec3 camera_position = Camera::GetPosition(env, ctx.jni_cache, camera.get());
  const float camera_pitch = Camera::GetPitch(env, ctx.jni_cache, camera.get());
  const float camera_yaw = Camera::GetYaw(env, ctx.jni_cache, camera.get());
  const float fov =
      GameRenderer::GetFov(env, ctx.jni_cache, game_renderer.get(), camera.get(), tick_delta);

  const auto local_player = Minecraft::GetLocalPlayer(env, ctx.jni_cache, minecraft_instance.get());

  std::array<float, 16> view_matrix = {};
  std::array<float, 16> projection_matrix = {};
  BuildViewMatrix(camera_position, camera_pitch, camera_yaw, view_matrix.data());
  BuildPerspectiveMatrix(fov,
                         static_cast<float>(ctx.display_width) /
                             static_cast<float>(ctx.display_height),
                         0.05f,
                         1000.0f,
                         projection_matrix.data());

  const double max_render_distance_sq =
      static_cast<double>(max_render_distance_) * static_cast<double>(max_render_distance_);
  ImDrawList* draw_list = ImGui::GetBackgroundDrawList();

  for (auto entity : ClientWorld::GetEntities(env, ctx.jni_cache, world.get())) {
    if (!entity) { continue; }
    if (local_player && env->IsSameObject(entity.get(), local_player.get()) == JNI_TRUE) {
      continue;
    }

    // extract coordinates first for early distance culling (3 jni calls).
    // avoids 6+ jni calls per entity when distance check fails.
    auto [entity_x, entity_y, entity_z] = Entity::GetCoordinates(env, ctx.jni_cache, entity.get());

    const double dx = entity_x - camera_position.x;
    const double dy = entity_y - camera_position.y;
    const double dz = entity_z - camera_position.z;
    const double distance_sq = dx * dx + dy * dy + dz * dz;
    if (distance_sq > max_render_distance_sq) { continue; }

    const std::array<float, 4>* box_color = nullptr;
    if (player_group_.enabled &&
        env->IsInstanceOf(entity.get(), ctx.jni_cache.client_player_entity_class)) {
      box_color = &player_group_.color;
    } else if (hostile_group_.enabled &&
               env->IsInstanceOf(entity.get(), ctx.jni_cache.hostile_entity_class)) {
      box_color = &hostile_group_.color;
    } else if (passive_group_.enabled &&
               env->IsInstanceOf(entity.get(), ctx.jni_cache.passive_entity_class)) {
      box_color = &passive_group_.color;
    } else if (item_group_.enabled &&
               env->IsInstanceOf(entity.get(), ctx.jni_cache.item_entity_class)) {
      box_color = &item_group_.color;
    }
    if (box_color == nullptr) { continue; }

    const EntityData entity_data = Entity::GetData(env, ctx.jni_cache, entity.get());
    if (!entity_data.is_alive) { continue; }

    const double lerped_x = Lerp(entity_data.prev_x, entity_x, tick_delta);
    const double lerped_y = Lerp(entity_data.prev_y, entity_y, tick_delta);
    const double lerped_z = Lerp(entity_data.prev_z, entity_z, tick_delta);

    const Vec3 feet = {lerped_x, lerped_y, lerped_z};
    const Vec3 head = {
        lerped_x, lerped_y + static_cast<double>(entity_data.height) + 0.2, lerped_z};

    Vec2 feet_screen = {};
    Vec2 head_screen = {};
    if (!WorldToScreen(feet,
                       view_matrix.data(),
                       projection_matrix.data(),
                       ctx.display_width,
                       ctx.display_height,
                       feet_screen) ||
        !WorldToScreen(head,
                       view_matrix.data(),
                       projection_matrix.data(),
                       ctx.display_width,
                       ctx.display_height,
                       head_screen)) {
      continue;
    }

    const float top = std::min(head_screen.y, feet_screen.y);
    const float bottom = std::max(head_screen.y, feet_screen.y);
    const float height = bottom - top;
    if (height < 4.0f) { continue; }

    const float half_width = height * 0.25f;
    draw_list->AddRect(ImVec2(head_screen.x - half_width, top),
                       ImVec2(head_screen.x + half_width, bottom),
                       ToImColor(*box_color),
                       0.0f,
                       0,
                       1.5f);
  }
}

}  // namespace mc_internal
