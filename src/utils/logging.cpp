#include "mc_internal/utils/logging.hpp"

namespace mc_internal {

void PrintStatus(std::string_view message) {
  std::print("{} {}\n", kLogPrefix, message);
  std::fflush(stdout);
}

}  // namespace mc_internal
