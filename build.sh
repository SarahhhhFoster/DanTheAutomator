#!/usr/bin/env bash
# macOS build + package.
# Usage: bash build.sh [--no-package]
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
SKIP_PACKAGE=0
[[ "${1:-}" == "--no-package" ]] && SKIP_PACKAGE=1

# ── Configure (first run fetches JUCE ~200 MB) ────────────────────────────────
if [ ! -f "$BUILD_DIR/CMakeCache.txt" ]; then
    echo "==> Configuring macOS build (first run — fetches JUCE ~200 MB)..."
    cmake -B "$BUILD_DIR" \
        -G "Unix Makefiles" \
        -DCMAKE_C_COMPILER=/usr/bin/clang \
        -DCMAKE_CXX_COMPILER=/usr/bin/clang++ \
        -DCMAKE_BUILD_TYPE=Release
fi

# ── Build ──────────────────────────────────────────────────────────────────────
echo "==> Building..."
cmake --build "$BUILD_DIR" --config Release -j"$(sysctl -n hw.logicalcpu)"
echo "==> Build done. Artefacts in build/MidiEnvelopePlugin_artefacts/Release/"

# ── Package ────────────────────────────────────────────────────────────────────
if [ "$SKIP_PACKAGE" -eq 0 ]; then
    echo ""
    bash "$SCRIPT_DIR/package-mac.sh"
fi
