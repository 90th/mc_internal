#include "mc_internal/ui/opengl_state.hpp"

#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES
#endif
#include <GL/gl.h>
#include <GL/glext.h>

namespace mc_internal {

void ResetOpenGlStateForImGuiBootstrap() {
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, 0);

  // forcefully unbind samplers and pbos to prevent out-of-bounds reads
  glBindSampler(0, 0);
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

  // completely scrub all unpack state from the game's texture uploads
  glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
  glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
  glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glPixelStorei(GL_UNPACK_SKIP_IMAGES, 0);
}

}  // namespace mc_internal
