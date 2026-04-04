#pragma once

namespace mc_internal::ui {

bool Tab(const char* label, bool selected);
bool SliderFloat(
    const char* label, float* v, float v_min, float v_max, const char* format = "%.2f");

}  // namespace mc_internal::ui