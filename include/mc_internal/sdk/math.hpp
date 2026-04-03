#pragma once

namespace mc_internal {

struct Vec2 {
  float x = 0.0f;
  float y = 0.0f;
};

struct Vec3 {
  double x = 0.0;
  double y = 0.0;
  double z = 0.0;
};

[[nodiscard]] bool WorldToScreen(const Vec3& world_pos,
                                 const float* view_matrix,
                                 const float* proj_matrix,
                                 int screen_width,
                                 int screen_height,
                                 Vec2& out_screen_pos);

}  // namespace mc_internal
