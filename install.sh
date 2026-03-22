#!/usr/bin/env bash
# Install Dan plugins locally and the Standalone app to /Applications.
# Plugin formats go to ~/Library (no sudo needed).
# /Applications and coreaudiod restart require admin — the macOS auth dialog
# will appear (works from Terminal, Finder, or any script context).
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ARTEFACTS="$SCRIPT_DIR/build/MidiEnvelopePlugin_artefacts/Release"

# Run a shell command with admin privileges via the native macOS auth dialog.
run_as_admin() {
    osascript -e "do shell script \"$*\" with administrator privileges"
}

# ── User-space plugins (~/Library — no sudo needed) ───────────────────────────
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
    xattr -rd com.apple.quarantine "$dest_dir/$name" 2>/dev/null || true
    codesign --force --deep -s - "$dest_dir/$name"
    echo "  ✓ $name"
}

echo "==> Installing Dan plugins (user-space)..."
install_plugin "$ARTEFACTS/VST3/Dan.vst3"    ~/Library/Audio/Plug-Ins/VST3
install_plugin "$ARTEFACTS/CLAP/Dan.clap"    ~/Library/Audio/Plug-Ins/CLAP
install_plugin "$ARTEFACTS/AU/Dan.component" ~/Library/Audio/Plug-Ins/Components

# ── AU rescan — coreaudiod restart (admin) ────────────────────────────────────
if [ -e ~/Library/Audio/Plug-Ins/Components/Dan.component ]; then
    echo "  Restarting coreaudiod so AU hosts rescan (admin auth dialog will appear)..."
    run_as_admin "killall coreaudiod" 2>/dev/null || true
    echo "  ✓ coreaudiod restarted"
fi

# ── Standalone → /Applications (admin — may be root-owned from PKG install) ───
APP_SRC="$ARTEFACTS/Standalone/Dan.app"
if [ ! -e "$APP_SRC" ]; then
    echo "  [skip] Dan.app not found in build artefacts"
else
    echo "  Installing Dan.app → /Applications (admin auth dialog will appear)..."
    # Escape the path for osascript in case it contains spaces
    ESCAPED_SRC="${APP_SRC//\'/\'\\\'\'}"
    run_as_admin \
        "cp -rf '${ESCAPED_SRC}' /Applications/" \
        "&& xattr -rd com.apple.quarantine /Applications/Dan.app" \
        "&& codesign --force --deep -s - /Applications/Dan.app"
    echo "  ✓ Dan.app"
fi

echo ""
echo "==> Done. Quit and relaunch your DAW to pick up the changes."
echo "    Ableton  : Plug-Ins → VST3 → Umaru Industrial Waste → Dan  (drag to MIDI track)"
echo "    Logic    : rescans AU on next launch"
echo "    Standalone: /Applications/Dan.app"
