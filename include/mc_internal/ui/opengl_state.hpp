#pragma once

namespace mc_internal {

// Resets OpenGL texture, sampler, PBO, and unpack state before ImGui
// initialization or frame setup to prevent interference from the game's
// texture pipeline.
void ResetOpenGlStateForImGuiBootstrap();

}  // namespace mc_internal
