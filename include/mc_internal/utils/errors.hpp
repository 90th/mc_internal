#pragma once

#include <type_traits>
#include <utility>

namespace mc_internal {

enum class BootstrapError {
  kThreadAttachFailed,
  kSwapBuffersLookupFailed,
  kSwapBuffersHookFailed,
};

template <typename T>
concept EnumLike = std::is_enum_v<T>;

template <EnumLike T>
constexpr auto ToUnderlying(T value) noexcept {
  return std::to_underlying(value);
}

}  // namespace mc_internal
