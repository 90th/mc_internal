#pragma once

#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES
#endif
#include <GL/gl.h>
#include <GL/glext.h>

namespace mc_internal {

struct OpenGlBootstrapState {
  GLint active_texture = 0;
  GLint texture_binding_2d = 0;
  GLint sampler_binding = 0;
  GLint pixel_unpack_buffer_binding = 0;
  GLint unpack_row_length = 0;
  GLint unpack_skip_rows = 0;
  GLint unpack_skip_pixels = 0;
  GLint unpack_alignment = 4;
  GLint unpack_skip_images = 0;
  GLint unpack_swap_bytes = 0;
  GLint unpack_lsb_first = 0;
};

// Temporarily normalizes texture upload state for ImGui bootstrap work and
// restores the game's previous state afterward.
[[nodiscard]] OpenGlBootstrapState PrepareOpenGlStateForImGuiBootstrap();
void RestoreOpenGlStateAfterImGuiBootstrap(const OpenGlBootstrapState& state);

}  // namespace mc_internal