#pragma once

namespace mc_internal::ui {

// Underline-style tab control. Returns true when clicked.
bool Tab(const char* label, bool selected);

// Custom slider with filled track and inline value display.
bool SliderFloat(
    const char* label, float* v, float v_min, float v_max, const char* format = "%.2f");

// Compact toggle switch (rectangular, not pill). Returns true when value changed.
bool Toggle(const char* label, bool* v);

// Section label with dimmed text and a thin rule underneath.
void SectionHeader(const char* label);

// Small descriptive text, rendered at reduced opacity.
void DescriptionText(const char* text);

}  // namespace mc_internal::ui
