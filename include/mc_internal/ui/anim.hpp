#pragma once

#include <algorithm>
#include <unordered_map>

#include "imgui.h"

namespace mc_internal::ui {

// Minimal keyed animation helper. Stores a float per ImGuiID and lerps it
// toward a target each frame at a fixed speed. Frame-rate independent via
// DeltaTime. Designed to be called inline from widget code:
//
//   float t = Anim::Get(id, target, speed);
//
// `target` is typically 0.0f or 1.0f.  `speed` is in units/second (12.0f is
// snappy, 6.0f is gentle).  Returns the current animated value [0..1].
struct Anim {
  static float Get(ImGuiID id, float target, float speed = 10.0f) {
    auto& val = Store()[id];
    const float dt = ImGui::GetIO().DeltaTime;
    if (val < target) {
      val = std::min(val + speed * dt, target);
    } else if (val > target) {
      val = std::max(val - speed * dt, target);
    }
    return val;
  }

  // Lerp variant: returns a value that smoothly approaches `target` each
  // frame.  `rate` ~10-15 gives a snappy feel; ~5-6 is more gentle.
  static float Lerp(ImGuiID id, float target, float rate = 12.0f) {
    auto& val = Store()[id];
    const float dt = ImGui::GetIO().DeltaTime;
    val += (target - val) * std::min(rate * dt, 1.0f);
    return val;
  }

 private:
  static std::unordered_map<ImGuiID, float>& Store() {
    static std::unordered_map<ImGuiID, float> s;
    return s;
  }
};

// Convenience: linearly interpolate between two ImU32 colors by t [0..1].
inline ImU32 LerpColor(ImU32 a, ImU32 b, float t) {
  ImVec4 ca = ImGui::ColorConvertU32ToFloat4(a);
  ImVec4 cb = ImGui::ColorConvertU32ToFloat4(b);
  return ImGui::ColorConvertFloat4ToU32(ImVec4(ca.x + (cb.x - ca.x) * t,
                                               ca.y + (cb.y - ca.y) * t,
                                               ca.z + (cb.z - ca.z) * t,
                                               ca.w + (cb.w - ca.w) * t));
}

}  // namespace mc_internal::ui
