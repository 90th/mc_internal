#include <iostream>
#include <limits>
#include <string_view>

#include "mc_internal/features/aim_assist_detail.hpp"

namespace {

bool Expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << message << '\n';
    return false;
  }
  return true;
}

}  // namespace

int main() {
  using mc_internal::Vec3;
  using mc_internal::aim_assist_detail::AimPoint;
  using mc_internal::aim_assist_detail::BuildAimPoint;
  using mc_internal::aim_assist_detail::CanResolveHostileTargetKeys;
  using mc_internal::aim_assist_detail::CanUseLockedTargetFastPath;
  using mc_internal::aim_assist_detail::IsAimPointValid;
  using mc_internal::aim_assist_detail::IsRuntimeConfigValid;
  using mc_internal::aim_assist_detail::PassesHardFov;
  using mc_internal::aim_assist_detail::PlanTargetSearch;
  using mc_internal::aim_assist_detail::ShouldContinueFovSampling;

  bool ok = true;

  {
    const auto search_plan = PlanTargetSearch(false, true, 27);
    ok &= Expect(!search_plan.prefer_locked_target,
                 "fresh activation should search any target even with stale lock state");
    ok &= Expect(search_plan.requested_target_id == 0,
                 "fresh activation should not request a locked target");
    ok &= Expect(!search_plan.retry_without_lock,
                 "fresh activation should not require a fallback scan");
  }

  {
    const auto search_plan = PlanTargetSearch(true, false, 0);
    ok &= Expect(!search_plan.prefer_locked_target,
                 "held activation without a lock should still search any target");
    ok &= Expect(search_plan.requested_target_id == 0,
                 "held activation without a lock should not request a locked target id");
    ok &= Expect(!search_plan.retry_without_lock,
                 "held activation without a lock should not dead-end on a missing target id");
  }

  {
    const auto search_plan = PlanTargetSearch(true, true, 0);
    ok &= Expect(search_plan.prefer_locked_target,
                 "entity id zero must remain a valid locked-target candidate");
    ok &= Expect(search_plan.requested_target_id == 0,
                 "search plan must preserve a locked entity id of zero");
    ok &= Expect(search_plan.retry_without_lock,
                 "held activation with a lock should fall back to reacquisition when needed");
  }

  {
    const auto search_plan = PlanTargetSearch(true, true, 42);
    ok &= Expect(search_plan.prefer_locked_target,
                 "held activation with a lock should prefer the locked target first");
    ok &= Expect(search_plan.requested_target_id == 42,
                 "search plan must preserve the current locked target id");
    ok &= Expect(search_plan.retry_without_lock,
                 "held activation with a lock should fall back to reacquisition when needed");
  }

  {
    const auto search_plan = PlanTargetSearch(true, true, 42);
    ok &= Expect(CanUseLockedTargetFastPath(search_plan, true),
                 "locked-target search should use the direct lookup path when the method exists");
    ok &=
        Expect(!CanUseLockedTargetFastPath(search_plan, false),
               "locked-target search should fall back to iteration when direct lookup is missing");
    ok &= Expect(!CanUseLockedTargetFastPath(PlanTargetSearch(true, false, 42), true),
                 "unlocked scans should not use the direct lookup path");
  }

  {
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const AimPoint point = BuildAimPoint(Vec3{nan, 0.0, 0.0}, 0.0f, 0.0f, Vec3{1.0, 0.0, 1.0});
    ok &= Expect(!IsAimPointValid(point), "BuildAimPoint should reject non-finite camera input");
  }

  {
    const AimPoint desired = BuildAimPoint(Vec3{0.0, 0.0, 0.0}, 0.0f, 0.0f, Vec3{10.0, 0.0, 1.0});
    ok &= Expect(IsAimPointValid(desired),
                 "BuildAimPoint should produce a valid aim point for finite inputs");
    ok &= Expect(!PassesHardFov(desired, 25.0f),
                 "hard FOV gate must reject desired aim points outside the configured limit");

    AimPoint ranking_only = desired;
    ranking_only.fov_delta = 0.0f;
    ok &= Expect(PassesHardFov(ranking_only, 25.0f),
                 "ranking-only FOV surrogates should not replace the desired-point hard gate");
  }

  {
    ok &= Expect(ShouldContinueFovSampling(std::numeric_limits<float>::max()),
                 "sampling should continue while no candidate has improved the score");
    ok &= Expect(!ShouldContinueFovSampling(0.0f),
                 "sampling should stop once the best possible fov delta is reached");
    ok &= Expect(!ShouldContinueFovSampling(-1.0f),
                 "sampling should stop for any zero-or-better terminal fov delta");
  }

  {
    ok &= Expect(IsRuntimeConfigValid(25.0f, 8.0f, 96.0f),
                 "normal runtime config should remain valid");
    ok &= Expect(!IsRuntimeConfigValid(25.0f, 0.0f, 96.0f),
                 "zero smoothness must be rejected to avoid invalid rotation steps");
    ok &= Expect(!IsRuntimeConfigValid(25.0f, std::numeric_limits<float>::quiet_NaN(), 96.0f),
                 "non-finite smoothness must be rejected");
    ok &=
        Expect(!IsRuntimeConfigValid(25.0f, 8.0f, -1.0f), "negative max distance must be rejected");
  }

  {
    ok &= Expect(CanResolveHostileTargetKeys(false, false, false),
                 "disabled hostile targeting should not require translation metadata");
    ok &= Expect(CanResolveHostileTargetKeys(true, true, true),
                 "hostile targeting should run when both JNI members are available");
    ok &= Expect(!CanResolveHostileTargetKeys(true, false, true),
                 "hostile targeting should fail closed when entity type lookup is missing");
    ok &= Expect(!CanResolveHostileTargetKeys(true, true, false),
                 "hostile targeting should fail closed when translation key lookup is missing");
  }

  if (!ok) { return 1; }
  return 0;
}
