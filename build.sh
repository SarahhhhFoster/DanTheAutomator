#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"

# Configure if the build directory doesn't exist yet
if [ ! -f "$BUILD_DIR/CMakeCache.txt" ]; then
    echo "==> Configuring (first run — fetches JUCE ~200 MB)..."
    cmake -B "$BUILD_DIR" \
        -G "Unix Makefiles" \
        -DCMAKE_C_COMPILER=/usr/bin/clang \
        -DCMAKE_CXX_COMPILER=/usr/bin/clang++ \
        -DCMAKE_BUILD_TYPE=Release
fi

echo "==> Building..."
cmake --build "$BUILD_DIR" --config Release -j"$(sysctl -n hw.logicalcpu)"
echo "==> Done. Artefacts in build/MidiEnvelopePlugin_artefacts/Release/"
