#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ARTEFACTS="$SCRIPT_DIR/build/MidiEnvelopePlugin_artefacts/Release"

install_plugin() {
    local src="$1"
    local dest_dir="$2"
    local name
    name="$(basename "$src")"

    if [ ! -e "$src" ]; then
        echo "  [skip] $name not found in build artefacts"
        return
    fi

    mkdir -p "$dest_dir"
    echo "  Copying $name → $dest_dir"
    cp -r "$src" "$dest_dir/"

    echo "  Clearing quarantine..."
    xattr -rd com.apple.quarantine "$dest_dir/$name" 2>/dev/null || true

    echo "  Re-signing (seals bundle resources after JUCE post-build steps)..."
    codesign --force --deep -s - "$dest_dir/$name"

    echo "  ✓ $name installed"
}

echo "==> Installing Dan plugins..."

install_plugin "$ARTEFACTS/VST3/Dan.vst3"       ~/Library/Audio/Plug-Ins/VST3
install_plugin "$ARTEFACTS/AU/Dan.component"    ~/Library/Audio/Plug-Ins/Components
install_plugin "$ARTEFACTS/CLAP/Dan.clap"       ~/Library/Audio/Plug-Ins/CLAP

echo ""
echo "==> All done. Quit and relaunch your DAW to pick up the changes."
echo "    Ableton: Plug-Ins → VST3 → Umaru Industrial Waste → Dan"
echo "    (drag onto a MIDI track)"
