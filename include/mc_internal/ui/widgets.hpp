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

// Persistent state for a FilteredChecklist instance. Caller owns one of these
// per checklist and passes it by reference each frame.
struct FilteredChecklistState {
  char search_buf[64] = {};
  bool popup_open = false;
};

// Combo-like popup containing a search box, scrollable checkbox list, and
// show-all / hide-all footer buttons. Returns true if any checkbox changed.
//
//   id           - unique ImGui string id (e.g. "##hostile_filter")
//   state        - persistent per-widget state
//   labels       - display labels for each item
//   values       - parallel bool array (same count as labels)
//   count        - number of items
//   preview_width - width of the compact combo button
bool FilteredChecklist(const char* id,
                       FilteredChecklistState& state,
                       const char* const* labels,
                       bool* values,
                       int count,
                       float preview_width = 100.0f);

}  // namespace mc_internal::ui
