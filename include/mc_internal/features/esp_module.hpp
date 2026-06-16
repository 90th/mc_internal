#pragma once

#include <array>
#include <string_view>
#include <unordered_set>

#include "mc_internal/features/module.hpp"
#include "mc_internal/ui/widgets.hpp"

namespace mc_internal {

inline constexpr int kHostileMobCount = 37;
inline constexpr int kPassiveMobCount = 42;

class EspModule : public Module {
 public:
  EspModule();

  void on_render_settings(const OverlayContext& ctx) override;
  void on_render_3d(const OverlayContext& ctx) override;

 private:
  void RebuildHostileFilter();
  void RebuildPassiveFilter();
  bool AllHostilesVisible() const;

  struct TargetGroupState {
    bool enabled = false;
    std::array<float, 4> color = {1.0f, 1.0f, 1.0f, 1.0f};
    int style = 0;
  };

  float max_render_distance_ = 256.0f;
  bool show_distance_ = true;
  bool show_nametags_ = true;
  bool show_health_bars_ = true;
  TargetGroupState player_group_ = {true, {0.86f, 0.15f, 0.15f, 1.0f}, 0};
  TargetGroupState hostile_group_ = {true, {0.98f, 0.36f, 0.22f, 1.0f}, 0};
  TargetGroupState passive_group_ = {false, {0.24f, 0.78f, 0.42f, 1.0f}, 0};
  TargetGroupState item_group_ = {false, {0.92f, 0.76f, 0.20f, 1.0f}, 0};
  std::array<bool, kHostileMobCount> hostile_mob_visible_{};
  std::array<bool, kPassiveMobCount> passive_mob_visible_{};
  std::unordered_set<std::string_view> hidden_hostile_keys_;
  std::unordered_set<std::string_view> hidden_passive_keys_;
  bool hostile_filter_dirty_ = true;
  bool passive_filter_dirty_ = true;
  ui::FilteredChecklistState hostile_checklist_state_{};
  ui::FilteredChecklistState passive_checklist_state_{};
};

}  // namespace mc_internal
