#include "mc_internal/features/debug_hud_module.hpp"

#include "imgui.h"

#include "mc_internal/context.hpp"
#include "mc_internal/core/jvm_attachment.hpp"
#include "mc_internal/sdk/minecraft.hpp"

namespace mc_internal {

DebugHudModule::DebugHudModule() : Module("Debug HUD", "Displays the local player coordinates") {}

void DebugHudModule::on_render_ui(const OverlayContext& ctx) {
  ImGui::SetNextWindowPos(ImVec2(320.0f, 40.0f), ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowSize(ImVec2(240.0f, 0.0f), ImGuiCond_FirstUseEver);

  ImGui::Begin("debug hud");

  const auto attachment = AttachCurrentThread(ctx.jvm);
  if (!attachment) {
    ImGui::Text("Player: JNI unavailable");
    ImGui::End();
    return;
  }

  const JniEnv env(attachment->env());
  if (!ctx.jni_cache.Initialize(env)) {
    ImGui::Text("Player: JNI cache unavailable");
    ImGui::End();
    return;
  }

  auto minecraft_instance = Minecraft::GetInstance(env, ctx.jni_cache);
  if (!minecraft_instance) {
    ImGui::Text("Player: Not in game");
    ImGui::End();
    return;
  }

  auto player = Minecraft::GetLocalPlayer(env, ctx.jni_cache, minecraft_instance.get());
  if (!player) {
    ImGui::Text("Player: Not in game");
    ImGui::End();
    return;
  }

  const auto [x, y, z] = ClientPlayerEntity::GetCoordinates(env, ctx.jni_cache, player.get());
  ImGui::Text("Pos: %.2f, %.2f, %.2f", x, y, z);
  ImGui::End();
}

}  // namespace mc_internal
