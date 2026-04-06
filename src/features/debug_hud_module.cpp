#include "mc_internal/features/debug_hud_module.hpp"

#include <algorithm>
#include <cstdio>

#include "imgui.h"

#include "mc_internal/context.hpp"
#include "mc_internal/core/jvm_attachment.hpp"
#include "mc_internal/sdk/minecraft.hpp"

namespace mc_internal {

namespace {

constexpr ImU32 kHudBg = IM_COL32(18, 19, 24, 200);
constexpr ImU32 kHudBorder = IM_COL32(40, 41, 48, 255);
constexpr ImU32 kHudText = IM_COL32(180, 180, 180, 255);
constexpr ImU32 kHudLabel = IM_COL32(110, 110, 110, 255);
constexpr ImU32 kHudAccent = IM_COL32(200, 60, 60, 255);
constexpr float kHudRounding = 4.0f;
constexpr float kHudPadX = 10.0f;
constexpr float kHudPadY = 6.0f;
constexpr float kHudMargin = 12.0f;

ImU32 ApplyAlpha(ImU32 col, float alpha) {
  int a = static_cast<int>((col >> IM_COL32_A_SHIFT & 0xFF) * alpha);
  return (col & ~IM_COL32_A_MASK) | (static_cast<ImU32>(a) << IM_COL32_A_SHIFT);
}

}  // namespace

DebugHudModule::DebugHudModule()
    : Module("debug hud", "displays the local player coordinates", ModuleCategory::kMisc) {}

void DebugHudModule::on_render_ui(const OverlayContext& ctx) {
  // fade in/out like menu_alpha
  static float hud_alpha = 0.0f;
  hud_alpha += (is_enabled() ? 1.0f : -1.0f) * ImGui::GetIO().DeltaTime * 10.0f;
  hud_alpha = std::clamp(hud_alpha, 0.0f, 1.0f);
  if (hud_alpha <= 0.001f) { return; }

  const char* status_text = nullptr;
  char coord_buf[128] = {};

  if (!ctx.jvm_attachment) {
    status_text = "jni unavailable";
  } else {
    const JniEnv env(ctx.jvm_attachment->env());
    if (!ctx.jni_cache.Initialize(env)) {
      status_text = "jni cache unavailable";
    } else {
      auto mc = Minecraft::GetInstance(env, ctx.jni_cache);
      if (!mc) {
        status_text = "not in game";
      } else {
        auto player = Minecraft::GetLocalPlayer(env, ctx.jni_cache, mc.get());
        if (!player) {
          status_text = "not in game";
        } else {
          const auto [x, y, z] =
              ClientPlayerEntity::GetCoordinates(env, ctx.jni_cache, player.get());
          std::snprintf(coord_buf, sizeof(coord_buf), "%.1f  %.1f  %.1f", x, y, z);
        }
      }
    }
  }

  const char* line1 = "pos";
  const char* line2 = (coord_buf[0] != '\0') ? coord_buf : status_text;

  ImDrawList* dl = ImGui::GetForegroundDrawList();
  const ImVec2 line1_size = ImGui::CalcTextSize(line1);
  const ImVec2 line2_size = ImGui::CalcTextSize(line2);
  const float line_spacing = 2.0f;

  const float box_w = kHudPadX * 2.0f + std::max(line1_size.x, line2_size.x);
  const float box_h = kHudPadY * 2.0f + line1_size.y + line_spacing + line2_size.y;

  const ImVec2 box_min(kHudMargin, kHudMargin);
  const ImVec2 box_max(box_min.x + box_w, box_min.y + box_h);

  dl->AddRectFilled(box_min, box_max, ApplyAlpha(kHudBg, hud_alpha), kHudRounding);
  dl->AddRect(box_min, box_max, ApplyAlpha(kHudBorder, hud_alpha), kHudRounding, 0, 1.0f);

  dl->AddText(
      ImVec2(box_min.x + kHudPadX, box_min.y + kHudPadY), ApplyAlpha(kHudLabel, hud_alpha), line1);

  dl->AddText(ImVec2(box_min.x + kHudPadX, box_min.y + kHudPadY + line1_size.y + line_spacing),
              ApplyAlpha((coord_buf[0] != '\0') ? kHudText : kHudAccent, hud_alpha),
              line2);
}

}  // namespace mc_internal
