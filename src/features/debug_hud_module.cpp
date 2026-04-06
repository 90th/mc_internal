#include "mc_internal/features/debug_hud_module.hpp"

#include <cstdio>

#include "imgui.h"

#include "mc_internal/context.hpp"
#include "mc_internal/core/jvm_attachment.hpp"
#include "mc_internal/sdk/minecraft.hpp"

namespace mc_internal {

DebugHudModule::DebugHudModule()
    : Module("debug hud", "displays the local player coordinates", ModuleCategory::kMisc) {}

void DebugHudModule::on_render_ui(const OverlayContext& ctx) {
  ImGui::SetNextWindowPos(ImVec2(320.0f, 40.0f), ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowSize(ImVec2(240.0f, 0.0f), ImGuiCond_FirstUseEver);

  ImGui::Begin("debug hud");

  if (!ctx.jvm_attachment) {
    ImGui::TextUnformatted("player: jni unavailable");
    ImGui::End();
    return;
  }

  const JniEnv env(ctx.jvm_attachment->env());
  if (!ctx.jni_cache.Initialize(env)) {
    ImGui::TextUnformatted("player: jni cache unavailable");
    ImGui::End();
    return;
  }

  auto minecraft_instance = Minecraft::GetInstance(env, ctx.jni_cache);
  if (!minecraft_instance) {
    ImGui::TextUnformatted("player: not in game");
    ImGui::End();
    return;
  }

  auto player = Minecraft::GetLocalPlayer(env, ctx.jni_cache, minecraft_instance.get());
  if (!player) {
    ImGui::TextUnformatted("player: not in game");
    ImGui::End();
    return;
  }

  const auto [x, y, z] = ClientPlayerEntity::GetCoordinates(env, ctx.jni_cache, player.get());

  // Format the string locally to bypass ImGui varargs corruption
  char buffer[128];
  std::snprintf(buffer, sizeof(buffer), "pos: %.2f, %.2f, %.2f", x, y, z);
  ImGui::TextUnformatted(buffer);

  ImGui::End();
}

}  // namespace mc_internal