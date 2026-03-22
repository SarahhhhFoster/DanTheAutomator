#!/usr/bin/env bash
# Create a macOS installer PKG from the build artefacts.
# Installs VST3, AU, CLAP to /Library/Audio/Plug-Ins/ and the Standalone
# app to /Applications (system-wide).
#
# Optional: sign and notarize with your Developer ID.
#   DEVELOPER_ID="Developer ID Installer: Your Name (XXXXXXXXXX)" bash package-mac.sh
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ARTEFACTS="$SCRIPT_DIR/build/MidiEnvelopePlugin_artefacts/Release"
DIST="$SCRIPT_DIR/dist"
VERSION="0.1.0"
PKG_ID="com.umaruwaste.dan"
PKG_OUT="$DIST/Dan-${VERSION}-macOS.pkg"

# Optional Developer ID Installer certificate (for signing the PKG itself).
# Leave empty to produce an unsigned PKG (still installs fine locally).
DEVELOPER_ID="${DEVELOPER_ID:-}"

echo "==> Packaging Dan ${VERSION} for macOS..."

# ── Stage plugins ──────────────────────────────────────────────────────────────
STAGING="$(mktemp -d)"
trap "rm -rf '$STAGING'" EXIT

VST3_DEST="$STAGING/Library/Audio/Plug-Ins/VST3"
AU_DEST="$STAGING/Library/Audio/Plug-Ins/Components"
CLAP_DEST="$STAGING/Library/Audio/Plug-Ins/CLAP"
APP_DEST="$STAGING/Applications"

stage_plugin() {
    local src="$1"
    local dest_dir="$2"
    local name
    name="$(basename "$src")"

    if [ ! -e "$src" ]; then
        echo "  [skip] $name not in build artefacts"
        return
    fi

    mkdir -p "$dest_dir"
    cp -r "$src" "$dest_dir/"
    xattr -rd com.apple.quarantine "$dest_dir/$name" 2>/dev/null || true
    codesign --force --deep -s - "$dest_dir/$name"
    echo "  Staged $name"
}

stage_plugin "$ARTEFACTS/VST3/Dan.vst3"              "$VST3_DEST"
stage_plugin "$ARTEFACTS/AU/Dan.component"           "$AU_DEST"
stage_plugin "$ARTEFACTS/CLAP/Dan.clap"              "$CLAP_DEST"
stage_plugin "$ARTEFACTS/Standalone/Dan.app"         "$APP_DEST"

# ── Create component PKG ───────────────────────────────────────────────────────
mkdir -p "$DIST"
COMPONENT_PKG="$(mktemp /tmp/dan-component-XXXXXX.pkg)"
trap "rm -f '$COMPONENT_PKG'; rm -rf '$STAGING'" EXIT

pkgbuild \
    --root             "$STAGING" \
    --install-location "/" \
    --identifier       "$PKG_ID" \
    --version          "$VERSION" \
    "$COMPONENT_PKG"

# ── Create distribution PKG (adds welcome/licence screens if desired) ──────────
if [ -n "$DEVELOPER_ID" ]; then
    echo "  Signing PKG with: $DEVELOPER_ID"
    productsign --sign "$DEVELOPER_ID" "$COMPONENT_PKG" "$PKG_OUT"
else
    cp "$COMPONENT_PKG" "$PKG_OUT"
fi

echo ""
echo "==> Installer: $PKG_OUT"
echo ""
echo "    Installs to:"
echo "      /Applications/Dan.app"
echo "      /Library/Audio/Plug-Ins/VST3/Dan.vst3"
echo "      /Library/Audio/Plug-Ins/Components/Dan.component"
echo "      /Library/Audio/Plug-Ins/CLAP/Dan.clap"
echo ""
if [ -z "$DEVELOPER_ID" ]; then
    echo "    NOTE: PKG is unsigned (ad-hoc plugin signing only)."
    echo "    To sign: DEVELOPER_ID=\"Developer ID Installer: ...\" bash package-mac.sh"
    echo "    To notarize after signing: xcrun notarytool submit ... --wait"
fi
