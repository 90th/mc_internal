#include "mc_internal/sdk/math.hpp"

namespace mc_internal {

bool WorldToScreen(const Vec3& world_pos,
                   const float* view_matrix,
                   const float* proj_matrix,
                   int screen_width,
                   int screen_height,
                   Vec2& out_screen_pos) {
  static_cast<void>(world_pos);
  static_cast<void>(view_matrix);
  static_cast<void>(proj_matrix);
  static_cast<void>(screen_width);
  static_cast<void>(screen_height);
  static_cast<void>(out_screen_pos);
  return false;
}

}  // namespace mc_internal
