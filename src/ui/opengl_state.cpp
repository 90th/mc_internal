#include "mc_internal/ui/opengl_state.hpp"

#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES
#endif
#include <GL/gl.h>
#include <GL/glext.h>

namespace mc_internal {

OpenGlBootstrapState PrepareOpenGlStateForImGuiBootstrap() {
  OpenGlBootstrapState state{};

  glGetIntegerv(GL_ACTIVE_TEXTURE, &state.active_texture);
  glActiveTexture(GL_TEXTURE0);
  glGetIntegerv(GL_TEXTURE_BINDING_2D, &state.texture_binding_2d);
  glGetIntegerv(GL_SAMPLER_BINDING, &state.sampler_binding);
  glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING, &state.pixel_unpack_buffer_binding);
  glGetIntegerv(GL_UNPACK_ROW_LENGTH, &state.unpack_row_length);
  glGetIntegerv(GL_UNPACK_SKIP_ROWS, &state.unpack_skip_rows);
  glGetIntegerv(GL_UNPACK_SKIP_PIXELS, &state.unpack_skip_pixels);
  glGetIntegerv(GL_UNPACK_ALIGNMENT, &state.unpack_alignment);
  glGetIntegerv(GL_UNPACK_SKIP_IMAGES, &state.unpack_skip_images);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, 0);
  glBindSampler(0, 0);
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
  glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
  glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
  glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glPixelStorei(GL_UNPACK_SKIP_IMAGES, 0);

  return state;
}

void RestoreOpenGlStateAfterImGuiBootstrap(const OpenGlBootstrapState& state) {
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(state.texture_binding_2d));
  glBindSampler(0, static_cast<GLuint>(state.sampler_binding));
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, static_cast<GLuint>(state.pixel_unpack_buffer_binding));
  glPixelStorei(GL_UNPACK_ROW_LENGTH, state.unpack_row_length);
  glPixelStorei(GL_UNPACK_SKIP_ROWS, state.unpack_skip_rows);
  glPixelStorei(GL_UNPACK_SKIP_PIXELS, state.unpack_skip_pixels);
  glPixelStorei(GL_UNPACK_ALIGNMENT, state.unpack_alignment);
  glPixelStorei(GL_UNPACK_SKIP_IMAGES, state.unpack_skip_images);
  glActiveTexture(state.active_texture);
}

}  // namespace mc_internal
