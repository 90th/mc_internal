#pragma once

#include "mc_internal/context.hpp"

namespace mc_internal {

// Ensures ImGui is initialized the first time this is called.
// Sets ctx.imgui_initialized on success or ctx.imgui_init_failed on failure.
// Returns true if ImGui is ready to render.
[[nodiscard]] bool EnsureImGuiInitialized(OverlayContext& ctx);

// Builds the font atlas texture and logs its dimensions.
void ForceBuildAndLogFontAtlasTexture();

}  // namespace mc_internal
