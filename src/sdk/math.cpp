#include "mc_internal/sdk/math.hpp"

#include <cmath>

namespace mc_internal {

namespace {

void MultiplyMatrixVector(const float* matrix, const float in[4], float out[4]) {
  out[0] = matrix[0] * in[0] + matrix[4] * in[1] + matrix[8] * in[2] + matrix[12] * in[3];
  out[1] = matrix[1] * in[0] + matrix[5] * in[1] + matrix[9] * in[2] + matrix[13] * in[3];
  out[2] = matrix[2] * in[0] + matrix[6] * in[1] + matrix[10] * in[2] + matrix[14] * in[3];
  out[3] = matrix[3] * in[0] + matrix[7] * in[1] + matrix[11] * in[2] + matrix[15] * in[3];
}

}  // namespace

bool WorldToScreen(const Vec3& world_pos,
                   const float* view_matrix,
                   const float* proj_matrix,
                   int screen_width,
                   int screen_height,
                   Vec2& out_screen_pos) {
  if (view_matrix == nullptr || proj_matrix == nullptr || screen_width <= 0 || screen_height <= 0) {
    return false;
  }

  const float world[4] = {static_cast<float>(world_pos.x),
                          static_cast<float>(world_pos.y),
                          static_cast<float>(world_pos.z),
                          1.0f};
  float view[4] = {};
  float clip[4] = {};

  MultiplyMatrixVector(view_matrix, world, view);
  MultiplyMatrixVector(proj_matrix, view, clip);

  if (clip[3] < 0.1f) { return false; }

  const float ndc_x = clip[0] / clip[3];
  const float ndc_y = clip[1] / clip[3];

  out_screen_pos.x = (ndc_x * 0.5f + 0.5f) * static_cast<float>(screen_width);
  out_screen_pos.y = (1.0f - (ndc_y * 0.5f + 0.5f)) * static_cast<float>(screen_height);
  return std::isfinite(out_screen_pos.x) && std::isfinite(out_screen_pos.y);
}

}  // namespace mc_internal
