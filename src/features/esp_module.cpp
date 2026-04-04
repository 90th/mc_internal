#include "mc_internal/features/esp_module.hpp"

#include <array>
#include <cmath>

#include "imgui.h"

#include "mc_internal/context.hpp"
#include "mc_internal/core/jvm_attachment.hpp"
#include "mc_internal/sdk/math.hpp"
#include "mc_internal/sdk/minecraft.hpp"

namespace mc_internal {

namespace {

constexpr float kPi = 3.14159265358979323846f;
constexpr float kMinRenderDistance = 16.0f;
constexpr float kMaxRenderDistance = 512.0f;

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

}  // namespace

EspModule::EspModule()
    : Module("entity esp",
             "draws projected 2d boxes around world entities",
             ModuleCategory::kVisuals) {}

void EspModule::on_render_settings(const OverlayContext& ctx) {
  static_cast<void>(ctx);

  ImGui::AlignTextToFramePadding();
  ImGui::TextUnformatted("entity boxes");
  if (ImGui::IsItemHovered()) { ImGui::SetTooltip("baseline visuals"); }

  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();

  if (ImGui::BeginTable("esp_settings_table",
                        2,
                        ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_BordersInnerV)) {
    ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed, 150.0f);
    ImGui::TableSetupColumn("value", ImGuiTableColumnFlags_WidthStretch);

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("target group");
    if (ImGui::IsItemHovered()) { ImGui::SetTooltip("this keeps the first pass simple"); }
    ImGui::TableSetColumnIndex(1);
    ImGui::Checkbox("show players", &show_players_);

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("max distance");
    if (ImGui::IsItemHovered()) { ImGui::SetTooltip("this trims visual noise and draw cost"); }
    ImGui::TableSetColumnIndex(1);
    ImGui::PushItemWidth(-1.0f);
    ImGui::SliderFloat("##esp_max_render_distance",
                       &max_render_distance_,
                       kMinRenderDistance,
                       kMaxRenderDistance,
                       "%.0f blocks");
    ImGui::PopItemWidth();

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("box tint");
    if (ImGui::IsItemHovered()) { ImGui::SetTooltip("this is the shared color for the base pass"); }
    ImGui::TableSetColumnIndex(1);
    ImGui::PushItemWidth(-1.0f);

    // Disable the corrupted tooltip on the color picker entirely
    ImGui::ColorEdit4("##esp_color",
                      esp_color_.data(),
                      ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_AlphaBar |
                          ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoTooltip);
    ImGui::PopItemWidth();

    ImGui::EndTable();
  }

  ImGui::Spacing();
}

void EspModule::on_render_3d(const OverlayContext& ctx) {
  if (!show_players_ || ctx.display_width <= 0 || ctx.display_height <= 0) { return; }

  const auto attachment = AttachCurrentThread(ctx.jvm);
  if (!attachment) { return; }

  const JniEnv env(attachment->env());
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
  const ImU32 esp_color = ImGui::ColorConvertFloat4ToU32(
      ImVec4(esp_color_[0], esp_color_[1], esp_color_[2], esp_color_[3]));
  ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
  for (auto entity : ClientWorld::GetEntities(env, ctx.jni_cache, world.get())) {
    if (!entity) { continue; }
    if (local_player && env->IsSameObject(entity.get(), local_player.get()) == JNI_TRUE) {
      continue;
    }

    const EntityData entity_data = Entity::GetData(env, ctx.jni_cache, entity.get());
    if (!entity_data.is_alive) { continue; }

    const double entity_x = Lerp(entity_data.prev_x, entity_data.x, tick_delta);
    const double entity_y = Lerp(entity_data.prev_y, entity_data.y, tick_delta);
    const double entity_z = Lerp(entity_data.prev_z, entity_data.z, tick_delta);

    const double dx = entity_x - camera_position.x;
    const double dy = entity_y - camera_position.y;
    const double dz = entity_z - camera_position.z;
    const double distance_sq = dx * dx + dy * dy + dz * dz;
    if (distance_sq > max_render_distance_sq) { continue; }

    const Vec3 feet = {entity_x, entity_y, entity_z};
    const Vec3 head = {
        entity_x, entity_y + static_cast<double>(entity_data.height) + 0.2, entity_z};

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
                       esp_color,
                       0.0f,
                       0,
                       1.5f);
  }
}

}  // namespace mc_internal