#!/usr/bin/env bash
# Minimal build script for this project.
# Usage:
#   ./build.sh           # clean + configure + build (Release)
#   ./build.sh debug     # clean + configure + build (Debug)
# Environment:
#   SKIP_BUILD=1 ./build.sh   # clean + configure only (no compile) - useful for CI/editor

set -euo pipefail

MODE=Release
if [[ "${1:-}" == "debug" || "${1:-}" == "Debug" ]]; then
  MODE=Debug
fi

# Only clean if the first argument is 'clean'
if [[ "${1:-}" == "clean" ]]; then
  echo "[build.sh] Cleaning build directory..."
  rm -rf build
  shift
fi

echo "[build.sh] Mode: ${MODE}"

mkdir -p build

echo "[build.sh] Configuring (CMAKE_BUILD_TYPE=${MODE})..."
cmake -S . -B build -DCMAKE_BUILD_TYPE=${MODE} -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# Run clang-format over project sources BEFORE building
if [[ -z "${SKIP_FORMAT:-}" ]]; then
  if command -v clang-format >/dev/null 2>&1; then
    echo "[build.sh] Running clang-format on src/ and include/mc_internal/..."
    if [[ -n "$(find src include/mc_internal -type f \( -name '*.cpp' -o -name '*.c' -o -name '*.hpp' -o -name '*.h' \) -print -quit 2>/dev/null)" ]]; then
      find src include/mc_internal -type f \( -name '*.cpp' -o -name '*.c' -o -name '*.hpp' -o -name '*.h' \) -print0 | xargs -0 clang-format -i --style=file
      echo "[build.sh] Formatting complete."
    else
      echo "[build.sh] No source/header files found to format."
    fi
  else
    echo "[build.sh] clang-format not found; skipping formatting."
  fi
fi

if [[ -z "${SKIP_BUILD:-}" ]]; then
  echo "[build.sh] Building (config=${MODE})..."
  # Use parallel builds where possible
  cmake --build build --config ${MODE} -- -j"$(nproc)"
else
  echo "[build.sh] SKIP_BUILD set; skipping compile step."
fi

echo "[build.sh] Done."