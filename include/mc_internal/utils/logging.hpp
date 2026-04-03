#pragma once

#include <cstdio>
#include <print>
#include <string_view>

#include "mc_internal/utils/errors.hpp"

namespace mc_internal {

inline constexpr std::string_view kLogPrefix = "[mc_internal]";

constexpr std::string_view Describe(BootstrapError error) noexcept {
  switch (error) {
    case BootstrapError::kThreadAttachFailed:
      return "could not attach the bootstrap thread to the vm";
    case BootstrapError::kSwapBuffersLookupFailed:
      return "could not locate glfwswapbuffers";
    case BootstrapError::kSwapBuffersHookFailed:
      return "could not detour glfwswapbuffers";
  }
  return "unknown bootstrap error";
}

void PrintStatus(std::string_view message);

template <EnumLike T>
void PrintFailure(std::string_view stage, T error) {
  std::println("{} {} failed: {} ({})", kLogPrefix, stage, Describe(error), ToUnderlying(error));
  std::fflush(stdout);
}

}  // namespace mc_internal
