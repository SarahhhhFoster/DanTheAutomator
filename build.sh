#!/usr/bin/env bash
# macOS build + package.
# Usage: bash build.sh [--no-package]
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
ARTEFACTS_DIR="$BUILD_DIR/MidiEnvelopePlugin_artefacts"
SKIP_PACKAGE=0
[[ "${1:-}" == "--no-package" ]] && SKIP_PACKAGE=1

# ── Fix ownership if a previous sudo run left root-owned artefacts ────────────
# CMake's lipo step cannot overwrite files it doesn't own.
if [ -d "$ARTEFACTS_DIR" ] && \
   find "$ARTEFACTS_DIR" -user root -maxdepth 6 2>/dev/null | grep -q .; then
    echo "==> Fixing root-owned artefacts (requires sudo once)..."
    sudo chown -R "$(id -un)" "$ARTEFACTS_DIR"
fi

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

# Ensure artefacts are owned by the current user after a successful build
# (belt-and-suspenders: the cmake step itself may create new root-owned files
#  if cmake was ever invoked via sudo in this session).
if find "$ARTEFACTS_DIR" -user root -maxdepth 6 2>/dev/null | grep -q .; then
    sudo chown -R "$(id -un)" "$ARTEFACTS_DIR"
fi

echo "==> Build done. Artefacts in build/MidiEnvelopePlugin_artefacts/Release/"

# ── Package ────────────────────────────────────────────────────────────────────
if [ "$SKIP_PACKAGE" -eq 0 ]; then
    echo ""
    bash "$SCRIPT_DIR/package-mac.sh"
fi
