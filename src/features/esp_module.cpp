#include "mc_internal/features/esp_module.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <unordered_set>

#include "imgui.h"

#include "mc_internal/context.hpp"
#include "mc_internal/core/jvm_attachment.hpp"
#include "mc_internal/sdk/math.hpp"
#include "mc_internal/sdk/minecraft.hpp"
#include "mc_internal/ui/anim.hpp"
#include "mc_internal/ui/widgets.hpp"

namespace mc_internal {

namespace {

constexpr float kPi = 3.14159265358979323846f;
constexpr float kMinRenderDistance = 16.0f;
constexpr float kMaxRenderDistance = 512.0f;
constexpr float kMinBoxSize = 4.0f;
constexpr ImGuiColorEditFlags kGroupColorEditFlags =
    ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_AlphaPreview |
    ImGuiColorEditFlags_NoTooltip;

constexpr int kEspStyleCorner = 0;
constexpr int kEspStyleBox = 1;

struct HostileMobEntry {
  const char* translation_key;
  const char* display_label;
};

struct PassiveMobEntry {
  const char* translation_key;
  const char* display_label;
};

constexpr HostileMobEntry kHostileMobs[] = {
    {"entity.minecraft.blaze", "blaze"},
    {"entity.minecraft.bogged", "bogged"},
    {"entity.minecraft.breeze", "breeze"},
    {"entity.minecraft.cave_spider", "cave spider"},
    {"entity.minecraft.creaking", "creaking"},
    {"entity.minecraft.creeper", "creeper"},
    {"entity.minecraft.drowned", "drowned"},
    {"entity.minecraft.elder_guardian", "elder guardian"},
    {"entity.minecraft.ender_dragon", "ender dragon"},
    {"entity.minecraft.enderman", "enderman"},
    {"entity.minecraft.endermite", "endermite"},
    {"entity.minecraft.evoker", "evoker"},
    {"entity.minecraft.ghast", "ghast"},
    {"entity.minecraft.guardian", "guardian"},
    {"entity.minecraft.hoglin", "hoglin"},
    {"entity.minecraft.husk", "husk"},
    {"entity.minecraft.magma_cube", "magma cube"},
    {"entity.minecraft.phantom", "phantom"},
    {"entity.minecraft.piglin_brute", "piglin brute"},
    {"entity.minecraft.pillager", "pillager"},
    {"entity.minecraft.ravager", "ravager"},
    {"entity.minecraft.shulker", "shulker"},
    {"entity.minecraft.silverfish", "silverfish"},
    {"entity.minecraft.skeleton", "skeleton"},
    {"entity.minecraft.slime", "slime"},
    {"entity.minecraft.spider", "spider"},
    {"entity.minecraft.stray", "stray"},
    {"entity.minecraft.vex", "vex"},
    {"entity.minecraft.vindicator", "vindicator"},
    {"entity.minecraft.warden", "warden"},
    {"entity.minecraft.witch", "witch"},
    {"entity.minecraft.wither", "wither"},
    {"entity.minecraft.wither_skeleton", "wither skeleton"},
    {"entity.minecraft.zoglin", "zoglin"},
    {"entity.minecraft.zombie", "zombie"},
    {"entity.minecraft.zombie_villager", "zombie villager"},
    {"entity.minecraft.zombified_piglin", "zombified piglin"},
};
static_assert(std::size(kHostileMobs) == kHostileMobCount);

constexpr PassiveMobEntry kPassiveMobs[] = {
    {"entity.minecraft.allay", "allay"},
    {"entity.minecraft.armadillo", "armadillo"},
    {"entity.minecraft.axolotl", "axolotl"},
    {"entity.minecraft.bat", "bat"},
    {"entity.minecraft.bee", "bee"},
    {"entity.minecraft.camel", "camel"},
    {"entity.minecraft.cat", "cat"},
    {"entity.minecraft.chicken", "chicken"},
    {"entity.minecraft.cod", "cod"},
    {"entity.minecraft.cow", "cow"},
    {"entity.minecraft.dolphin", "dolphin"},
    {"entity.minecraft.donkey", "donkey"},
    {"entity.minecraft.fox", "fox"},
    {"entity.minecraft.frog", "frog"},
    {"entity.minecraft.glow_squid", "glow squid"},
    {"entity.minecraft.goat", "goat"},
    {"entity.minecraft.horse", "horse"},
    {"entity.minecraft.iron_golem", "iron golem"},
    {"entity.minecraft.llama", "llama"},
    {"entity.minecraft.mooshroom", "mooshroom"},
    {"entity.minecraft.mule", "mule"},
    {"entity.minecraft.ocelot", "ocelot"},
    {"entity.minecraft.panda", "panda"},
    {"entity.minecraft.parrot", "parrot"},
    {"entity.minecraft.pig", "pig"},
    {"entity.minecraft.polar_bear", "polar bear"},
    {"entity.minecraft.pufferfish", "pufferfish"},
    {"entity.minecraft.rabbit", "rabbit"},
    {"entity.minecraft.salmon", "salmon"},
    {"entity.minecraft.sheep", "sheep"},
    {"entity.minecraft.sniffer", "sniffer"},
    {"entity.minecraft.snow_golem", "snow golem"},
    {"entity.minecraft.squid", "squid"},
    {"entity.minecraft.strider", "strider"},
    {"entity.minecraft.tadpole", "tadpole"},
    {"entity.minecraft.trader_llama", "trader llama"},
    {"entity.minecraft.tropical_fish", "tropical fish"},
    {"entity.minecraft.turtle", "turtle"},
    {"entity.minecraft.villager", "villager"},
    {"entity.minecraft.wandering_trader", "wandering trader"},
    {"entity.minecraft.wolf", "wolf"},
    {"entity.minecraft.zombie_horse", "zombie horse"},
};
static_assert(std::size(kPassiveMobs) == kPassiveMobCount);

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

struct JniLocalFrame {
  JNIEnv* env;
  JniLocalFrame(JNIEnv* e, jint capacity) : env(e->PushLocalFrame(capacity) == 0 ? e : nullptr) {}
  ~JniLocalFrame() {
    if (env) env->PopLocalFrame(nullptr);
  }
  explicit operator bool() const { return env != nullptr; }
  JniLocalFrame(const JniLocalFrame&) = delete;
  JniLocalFrame& operator=(const JniLocalFrame&) = delete;
};

void DrawCornerEsp(ImDrawList* dl,
                   const ImVec2& mn,
                   const ImVec2& mx,
                   ImU32 color,
                   ImU32 shadow_color,
                   float thickness) {
  float cl = std::min(mx.x - mn.x, mx.y - mn.y) * 0.25f;
  cl = std::max(cl, 4.0f);

  auto draw = [&](ImU32 c, float t) {
    dl->AddLine(ImVec2(mn.x, mn.y), ImVec2(mn.x + cl, mn.y), c, t);
    dl->AddLine(ImVec2(mn.x, mn.y), ImVec2(mn.x, mn.y + cl), c, t);
    dl->AddLine(ImVec2(mx.x, mn.y), ImVec2(mx.x - cl, mn.y), c, t);
    dl->AddLine(ImVec2(mx.x, mn.y), ImVec2(mx.x, mn.y + cl), c, t);
    dl->AddLine(ImVec2(mn.x, mx.y), ImVec2(mn.x + cl, mx.y), c, t);
    dl->AddLine(ImVec2(mn.x, mx.y), ImVec2(mn.x, mx.y - cl), c, t);
    dl->AddLine(ImVec2(mx.x, mx.y), ImVec2(mx.x - cl, mx.y), c, t);
    dl->AddLine(ImVec2(mx.x, mx.y), ImVec2(mx.x, mx.y - cl), c, t);
  };

  draw(shadow_color, thickness + 2.0f);
  draw(color, thickness);
}

}  // namespace

EspModule::EspModule()
    : Module("entity esp",
             "draws projected 2d boxes around world entities",
             ModuleCategory::kVisuals) {
  hostile_mob_visible_.fill(true);
  passive_mob_visible_.fill(true);
}

void EspModule::RebuildHostileFilter() {
  hidden_hostile_keys_.clear();
  for (int i = 0; i < kHostileMobCount; ++i) {
    if (!hostile_mob_visible_[i]) { hidden_hostile_keys_.insert(kHostileMobs[i].translation_key); }
  }
  hostile_filter_dirty_ = false;
}

void EspModule::RebuildPassiveFilter() {
  hidden_passive_keys_.clear();
  for (int i = 0; i < kPassiveMobCount; ++i) {
    if (!passive_mob_visible_[i]) { hidden_passive_keys_.insert(kPassiveMobs[i].translation_key); }
  }
  passive_filter_dirty_ = false;
}

bool EspModule::AllHostilesVisible() const {
  for (bool v : hostile_mob_visible_) {
    if (!v) return false;
  }
  return true;
}

bool IsTrackedHostileKey(std::string_view key) {
  for (const auto& hostile : kHostileMobs) {
    if (key == hostile.translation_key) { return true; }
  }
  return false;
}

bool IsTrackedPassiveKey(std::string_view key) {
  for (const auto& passive : kPassiveMobs) {
    if (key == passive.translation_key) { return true; }
  }
  return false;
}

bool IsPassiveClassMatch(JNIEnv* env, const JniCache& cache, jobject entity) {
  if (cache.passive_entity_class != nullptr &&
      env->IsInstanceOf(entity, cache.passive_entity_class)) {
    return true;
  }
  if (cache.golem_entity_class != nullptr && env->IsInstanceOf(entity, cache.golem_entity_class)) {
    return true;
  }
  if (cache.villager_entity_class != nullptr &&
      env->IsInstanceOf(entity, cache.villager_entity_class)) {
    return true;
  }
  if (cache.merchant_entity_class != nullptr &&
      env->IsInstanceOf(entity, cache.merchant_entity_class)) {
    return true;
  }
  if (cache.water_creature_entity_class != nullptr &&
      env->IsInstanceOf(entity, cache.water_creature_entity_class)) {
    return true;
  }
  if (cache.ambient_entity_class != nullptr &&
      env->IsInstanceOf(entity, cache.ambient_entity_class)) {
    return true;
  }
  if (cache.animal_entity_class != nullptr &&
      env->IsInstanceOf(entity, cache.animal_entity_class)) {
    return true;
  }
  return false;
}

void EspModule::on_render_settings(const OverlayContext& ctx) {
  static_cast<void>(ctx);

  ui::SectionHeader("target groups");
  ui::DescriptionText(
      "separate entity buckets by color and box style so crowded scenes stay readable.");
  ImGui::Spacing();

  const float region_x = ImGui::GetCursorPosX();
  const float avail = ImGui::GetContentRegionAvail().x;
  const float filter_x = region_x + 94.0f;
  const float combo_x = region_x + 202.0f;
  const float color_x = region_x + avail - ImGui::GetFrameHeight();

  ui::TargetGroupRow("players",
                     "##esp_players_style",
                     "##esp_players_color",
                     &player_group_.enabled,
                     player_group_.color.data(),
                     &player_group_.style,
                     combo_x,
                     color_x,
                     kGroupColorEditFlags);

  ImGui::Checkbox("hostiles", &hostile_group_.enabled);
  if (hostile_group_.enabled) {
    static const char* hostile_labels[kHostileMobCount];
    static bool hostile_labels_init = false;
    if (!hostile_labels_init) {
      for (int i = 0; i < kHostileMobCount; ++i) {
        hostile_labels[i] = kHostileMobs[i].display_label;
      }
      hostile_labels_init = true;
    }

    ImGui::SameLine();
    ImGui::SetCursorPosX(filter_x);
    if (ui::FilteredChecklist("##esp_hostile_filter",
                              hostile_checklist_state_,
                              hostile_labels,
                              hostile_mob_visible_.data(),
                              kHostileMobCount,
                              100.0f)) {
      hostile_filter_dirty_ = true;
    }

    ImGui::SameLine();
    ImGui::SetCursorPosX(combo_x);
    ImGui::PushItemWidth(72.0f);
    ImGui::Combo("##esp_hostile_style", &hostile_group_.style, "corner\0box\0");
    ImGui::PopItemWidth();
  }
  ImGui::SameLine();
  ImGui::SetCursorPosX(color_x);
  ImGui::ColorEdit4("##esp_hostiles_color", hostile_group_.color.data(), kGroupColorEditFlags);

  ImGui::Checkbox("passives", &passive_group_.enabled);
  if (passive_group_.enabled) {
    static const char* passive_labels[kPassiveMobCount];
    static bool passive_labels_init = false;
    if (!passive_labels_init) {
      for (int i = 0; i < kPassiveMobCount; ++i) {
        passive_labels[i] = kPassiveMobs[i].display_label;
      }
      passive_labels_init = true;
    }

    ImGui::SameLine();
    ImGui::SetCursorPosX(filter_x);
    if (ui::FilteredChecklist("##esp_passive_filter",
                              passive_checklist_state_,
                              passive_labels,
                              passive_mob_visible_.data(),
                              kPassiveMobCount,
                              100.0f)) {
      passive_filter_dirty_ = true;
    }

    ImGui::SameLine();
    ImGui::SetCursorPosX(combo_x);
    ImGui::PushItemWidth(72.0f);
    ImGui::Combo("##esp_passives_style", &passive_group_.style, "corner\0box\0");
    ImGui::PopItemWidth();
  }
  ImGui::SameLine();
  ImGui::SetCursorPosX(color_x);
  ImGui::ColorEdit4("##esp_passives_color", passive_group_.color.data(), kGroupColorEditFlags);

  ui::TargetGroupRow("items",
                     "##esp_items_style",
                     "##esp_items_color",
                     &item_group_.enabled,
                     item_group_.color.data(),
                     &item_group_.style,
                     combo_x,
                     color_x,
                     kGroupColorEditFlags);

  ui::SectionHeader("overlay tuning");
  ui::DescriptionText("adjust how far esp reaches and which labels are worth drawing every frame.");
  ImGui::Spacing();

  ui::LabeledSlider("render distance",
                    "##esp_max_render_distance",
                    &max_render_distance_,
                    kMinRenderDistance,
                    kMaxRenderDistance,
                    "%.0f blocks");

  ImGui::Spacing();
  ui::Toggle("show nametags", &show_nametags_);
  ui::Toggle("show health bars", &show_health_bars_);
  ui::Toggle("show distance", &show_distance_);
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

  if (hostile_group_.enabled && hostile_filter_dirty_) { RebuildHostileFilter(); }
  if (passive_group_.enabled && passive_filter_dirty_) { RebuildPassiveFilter(); }

  for (auto entity : ClientWorld::GetEntities(env, ctx.jni_cache, world.get())) {
    if (!entity) { continue; }
    JniLocalFrame frame(env.get(), 24);
    if (!frame) { continue; }

    if (local_player && env->IsSameObject(entity.get(), local_player.get()) == JNI_TRUE) {
      continue;
    }

    auto [entity_x, entity_y, entity_z] = Entity::GetCoordinates(env, ctx.jni_cache, entity.get());

    const double dx = entity_x - camera_position.x;
    const double dy = entity_y - camera_position.y;
    const double dz = entity_z - camera_position.z;
    const double distance_sq = dx * dx + dy * dy + dz * dz;
    if (distance_sq > max_render_distance_sq) { continue; }

    if (!Entity::IsAlive(env, ctx.jni_cache, entity.get())) { continue; }

    const bool needs_translation_key = hostile_group_.enabled || passive_group_.enabled;
    const std::string translation_key =
        needs_translation_key ? Entity::GetTranslationKey(env, ctx.jni_cache, entity.get())
                              : std::string();

    const TargetGroupState* group = nullptr;
    if (player_group_.enabled && ctx.jni_cache.player_entity_class != nullptr &&
        env->IsInstanceOf(entity.get(), ctx.jni_cache.player_entity_class)) {
      group = &player_group_;
    } else if (hostile_group_.enabled && !translation_key.empty() &&
               IsTrackedHostileKey(translation_key)) {
      if (!hidden_hostile_keys_.empty() &&
          hidden_hostile_keys_.count(std::string_view(translation_key)) > 0) {
        continue;
      }
      group = &hostile_group_;
    } else if (passive_group_.enabled &&
               ((!translation_key.empty() && IsTrackedPassiveKey(translation_key)) ||
                IsPassiveClassMatch(env.get(), ctx.jni_cache, entity.get()))) {
      if (!translation_key.empty() &&
          hidden_passive_keys_.count(std::string_view(translation_key)) > 0) {
        continue;
      }
      group = &passive_group_;
    } else if (item_group_.enabled &&
               env->IsInstanceOf(entity.get(), ctx.jni_cache.item_entity_class)) {
      group = &item_group_;
    }
    if (group == nullptr) { continue; }

    const EntityData entity_data = Entity::GetData(env, ctx.jni_cache, entity.get());

    const double lerped_x = Lerp(entity_data.prev_x, entity_x, tick_delta);
    const double lerped_y = Lerp(entity_data.prev_y, entity_y, tick_delta);
    const double lerped_z = Lerp(entity_data.prev_z, entity_z, tick_delta);

    float hw = entity_data.width * 0.5f;
    if (hw < 0.05f) hw = 0.3f;
    const double top_y = lerped_y + static_cast<double>(entity_data.height) + 0.1;

    const Vec3 corners[8] = {
        {lerped_x - hw, lerped_y, lerped_z - hw},
        {lerped_x + hw, lerped_y, lerped_z - hw},
        {lerped_x - hw, lerped_y, lerped_z + hw},
        {lerped_x + hw, lerped_y, lerped_z + hw},
        {lerped_x - hw, top_y, lerped_z - hw},
        {lerped_x + hw, top_y, lerped_z - hw},
        {lerped_x - hw, top_y, lerped_z + hw},
        {lerped_x + hw, top_y, lerped_z + hw},
    };

    float smin_x = 1e30f, smin_y = 1e30f;
    float smax_x = -1e30f, smax_y = -1e30f;
    int valid_count = 0;

    for (int i = 0; i < 8; ++i) {
      Vec2 sp;
      if (!WorldToScreenRelaxed(corners[i],
                                view_matrix.data(),
                                projection_matrix.data(),
                                ctx.display_width,
                                ctx.display_height,
                                sp)) {
        continue;
      }
      ++valid_count;
      smin_x = std::min(smin_x, sp.x);
      smin_y = std::min(smin_y, sp.y);
      smax_x = std::max(smax_x, sp.x);
      smax_y = std::max(smax_y, sp.y);
    }

    if (valid_count == 0) { continue; }

    const float sw = static_cast<float>(ctx.display_width);
    const float sh = static_cast<float>(ctx.display_height);
    if (smax_x < 0.0f || smin_x > sw || smax_y < 0.0f || smin_y > sh) { continue; }

    if (smax_x - smin_x < 1.0f || smax_y - smin_y < 1.0f) { continue; }

    const float proj_w = smax_x - smin_x;
    const float proj_h = smax_y - smin_y;
    const float size_alpha = std::clamp(std::min(proj_w, proj_h) / kMinBoxSize, 0.0f, 1.0f);

    if (proj_w < kMinBoxSize) {
      const float cx = (smin_x + smax_x) * 0.5f;
      smin_x = cx - kMinBoxSize * 0.5f;
      smax_x = cx + kMinBoxSize * 0.5f;
    }
    if (proj_h < kMinBoxSize) {
      const float cy = (smin_y + smax_y) * 0.5f;
      smin_y = cy - kMinBoxSize * 0.5f;
      smax_y = cy + kMinBoxSize * 0.5f;
    }

    const float distance = std::sqrt(static_cast<float>(distance_sq));
    float alpha_mult = 1.0f;
    const float fade_start = max_render_distance_ * 0.5f;
    if (distance > fade_start) {
      alpha_mult = 1.0f - (distance - fade_start) / (max_render_distance_ - fade_start);
      alpha_mult = std::clamp(alpha_mult, 0.0f, 1.0f);
    }
    alpha_mult *= size_alpha;

    std::array<float, 4> adj_color = group->color;
    adj_color[3] *= alpha_mult;
    const ImU32 im_color = ToImColor(adj_color);
    const ImU32 shadow_color = IM_COL32(0, 0, 0, static_cast<int>(180.0f * alpha_mult));

    const ImVec2 box_min(smin_x, smin_y);
    const ImVec2 box_max(smax_x, smax_y);

    if (group->style == kEspStyleCorner) {
      DrawCornerEsp(draw_list, box_min, box_max, im_color, shadow_color, 1.5f);
    } else {
      draw_list->AddRect(box_min, box_max, shadow_color, 0.0f, 0, 3.5f);
      draw_list->AddRect(box_min, box_max, im_color, 0.0f, 0, 1.5f);
    }

    float above_y = smin_y;

    if (show_health_bars_ && env->IsInstanceOf(entity.get(), ctx.jni_cache.living_entity_class)) {
      const float health = LivingEntity::GetHealth(env, ctx.jni_cache, entity.get());
      const float max_health = LivingEntity::GetMaxHealth(env, ctx.jni_cache, entity.get());

      if (max_health > 0.0f) {
        constexpr float kBarHeight = 2.0f;
        const float bar_width = smax_x - smin_x;
        const float bar_y = above_y - kBarHeight - 2.0f;
        const float health_ratio = std::clamp(health / max_health, 0.0f, 1.0f);
        const int eid = Entity::GetId(env, ctx.jni_cache, entity.get());

        draw_list->AddRectFilled(ImVec2(smin_x, bar_y),
                                 ImVec2(smax_x, bar_y + kBarHeight),
                                 IM_COL32(0, 0, 0, static_cast<int>(80.0f * alpha_mult)));

        const ImGuiID trail_id = static_cast<ImGuiID>(eid) ^ 0xDA47u;
        float trail_ratio = ui::Anim::Lerp(trail_id, health_ratio, 3.0f);
        trail_ratio = std::max(trail_ratio, health_ratio);
        if (trail_ratio > health_ratio + 0.001f) {
          draw_list->AddRectFilled(ImVec2(smin_x + bar_width * health_ratio, bar_y),
                                   ImVec2(smin_x + bar_width * trail_ratio, bar_y + kBarHeight),
                                   IM_COL32(140, 140, 140, static_cast<int>(160.0f * alpha_mult)));
        }

        if (health_ratio > 0.0f) {
          const float hr = std::clamp((1.0f - health_ratio) * 2.0f, 0.0f, 1.0f);
          const float hg = std::clamp(health_ratio * 2.0f, 0.0f, 1.0f);
          draw_list->AddRectFilled(ImVec2(smin_x, bar_y),
                                   ImVec2(smin_x + bar_width * health_ratio, bar_y + kBarHeight),
                                   IM_COL32(static_cast<int>(hr * 255),
                                            static_cast<int>(hg * 255),
                                            0,
                                            static_cast<int>(220.0f * alpha_mult)));
        }

        const float absorption =
            LivingEntity::GetAbsorptionAmount(env, ctx.jni_cache, entity.get());
        if (absorption > 0.0f) {
          const float abs_ratio = std::clamp(absorption / max_health, 0.0f, 1.0f - health_ratio);
          if (abs_ratio > 0.001f) {
            draw_list->AddRectFilled(
                ImVec2(smin_x + bar_width * health_ratio, bar_y),
                ImVec2(smin_x + bar_width * (health_ratio + abs_ratio), bar_y + kBarHeight),
                IM_COL32(255, 200, 40, static_cast<int>(200.0f * alpha_mult)));
          }
        }

        above_y = bar_y;
      }
    }

    if (show_nametags_) {
      const std::string name = Entity::GetName(env, ctx.jni_cache, entity.get());
      if (!name.empty()) {
        const ImVec2 name_size = ImGui::CalcTextSize(name.c_str());
        const float name_x = (smin_x + smax_x) * 0.5f - name_size.x * 0.5f;
        const float name_y = above_y - name_size.y - 2.0f;
        const ImU32 name_color = IM_COL32(255, 255, 255, static_cast<int>(255.0f * alpha_mult));
        draw_list->AddText(ImVec2(name_x + 1.0f, name_y + 1.0f), shadow_color, name.c_str());
        draw_list->AddText(ImVec2(name_x, name_y), name_color, name.c_str());
      }
    }

    if (show_distance_) {
      char dist_buf[16];
      std::snprintf(dist_buf, sizeof(dist_buf), "%.0f", distance);
      const ImVec2 text_size = ImGui::CalcTextSize(dist_buf);
      const float text_x = (smin_x + smax_x) * 0.5f - text_size.x * 0.5f;
      const float text_y = smax_y + 2.0f;
      draw_list->AddText(ImVec2(text_x + 1.0f, text_y + 1.0f), shadow_color, dist_buf);
      draw_list->AddText(ImVec2(text_x, text_y), im_color, dist_buf);
    }
  }
}

}  // namespace mc_internal
