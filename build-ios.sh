#!/usr/bin/env bash
# iOS build → archive → export IPA.
# Requires Xcode + an Apple Developer account.
#
# Usage:
#   bash build-ios.sh <TEAM_ID>
#   DAN_TEAM_ID=XXXXXXXXXX bash build-ios.sh
#
# FIRST-TIME SETUP (required before the first successful command-line build):
#   1. Connect your iPad to this Mac via USB
#   2. Run this script — it will configure CMake and then open the Xcode project
#   3. In Xcode: select each target → Signing & Capabilities → confirm your team
#      Xcode will register your device and create provisioning profiles automatically
#   4. Press ⌘R in Xcode to build and install once
#   5. After that, re-run this script for headless archive/IPA export
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build-ios"
DIST_DIR="$SCRIPT_DIR/dist"
ARCHIVE="$DIST_DIR/Dan-iOS.xcarchive"
XCPROJ="$BUILD_DIR/MidiEnvelopePlugin.xcodeproj"

TEAM_ID="${1:-${DAN_TEAM_ID:-}}"
if [ -z "$TEAM_ID" ]; then
    echo "ERROR: Apple Developer Team ID required."
    echo "Usage:  bash build-ios.sh <TEAM_ID>"
    echo "        DAN_TEAM_ID=XXXXXXXXXX bash build-ios.sh"
    echo ""
    echo "Find your Team ID at: https://developer.apple.com → Account → Membership"
    exit 1
fi

# ── Configure ─────────────────────────────────────────────────────────────────
if [ ! -f "$XCPROJ/project.pbxproj" ]; then
    echo "==> Configuring iOS build (first run — fetches JUCE ~200 MB)..."
    cmake -B "$BUILD_DIR" \
        -G Xcode \
        -DCMAKE_SYSTEM_NAME=iOS \
        -DCMAKE_OSX_DEPLOYMENT_TARGET=15.0 \
        -DDAN_TEAM_ID="$TEAM_ID"
    echo ""
    echo "==> Xcode project created at: $XCPROJ"
fi

# ── Check for connected device ────────────────────────────────────────────────
DEVICE_COUNT=$(xcrun xctrace list devices 2>/dev/null | grep -v "Simulator\|==" | grep -c "iPhone\|iPad" || true)

if [ "$DEVICE_COUNT" -eq 0 ]; then
    echo ""
    echo "======================================================================"
    echo "  FIRST-TIME SETUP REQUIRED"
    echo "======================================================================"
    echo ""
    echo "  No iPad/iPhone detected. Provisioning profiles cannot be created"
    echo "  without a registered device."
    echo ""
    echo "  To complete setup:"
    echo "    1. Connect your iPad to this Mac via USB"
    echo "    2. Trust this Mac on the iPad when prompted"
    echo "    3. Open the Xcode project:"
    echo ""
    echo "       open \"$XCPROJ\""
    echo ""
    echo "    4. In Xcode: select the MidiEnvelopePlugin_Standalone target"
    echo "       → Signing & Capabilities → Team: SS9U62UDA8"
    echo "       (Xcode will register your iPad and create provisioning profiles)"
    echo "    5. Do the same for MidiEnvelopePlugin_AUv3 target"
    echo "    6. Press ⌘R to build and install on your iPad once"
    echo "    7. Re-run this script to produce the distributable IPA"
    echo ""
    echo "  Opening Xcode project now..."
    open "$XCPROJ"
    exit 0
fi

echo "==> Found $DEVICE_COUNT connected device(s). Proceeding with build..."

# ── Archive ───────────────────────────────────────────────────────────────────
echo ""
echo "==> Archiving (this will take a few minutes)..."
mkdir -p "$DIST_DIR"

xcodebuild \
    -project "$XCPROJ" \
    -scheme  "MidiEnvelopePlugin_Standalone" \
    -configuration Release \
    -destination "generic/platform=iOS" \
    -archivePath "$ARCHIVE" \
    -allowProvisioningUpdates \
    archive \
    DEVELOPMENT_TEAM="$TEAM_ID" \
    CODE_SIGN_STYLE=Automatic \
    | grep -E "^(error:|warning: |==>|Build|Archive|\\*\\*)" || true

# xcodebuild exits non-zero on error; set -e will catch it above
echo "==> Archive: $ARCHIVE"

# ── Export ad-hoc IPA ─────────────────────────────────────────────────────────
echo ""
echo "==> Exporting ad-hoc IPA..."

EXPORT_OPTS="$(mktemp /tmp/dan-export-XXXXXX.plist)"
trap "rm -f '$EXPORT_OPTS'" EXIT

cat > "$EXPORT_OPTS" <<PLIST
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>method</key>
    <string>ad-hoc</string>
    <key>teamID</key>
    <string>${TEAM_ID}</string>
    <key>thinning</key>
    <string>&lt;none&gt;</string>
    <key>compileBitcode</key>
    <false/>
    <key>stripSwiftSymbols</key>
    <true/>
</dict>
</plist>
PLIST

xcodebuild -exportArchive \
    -archivePath       "$ARCHIVE" \
    -exportOptionsPlist "$EXPORT_OPTS" \
    -exportPath        "$DIST_DIR/" \
    -allowProvisioningUpdates

IPA="$DIST_DIR/Dan.ipa"
[ -f "$IPA" ] || IPA="$(find "$DIST_DIR" -name "*.ipa" | head -1)"

echo ""
echo "======================================================================"
echo "  BUILD COMPLETE"
echo "======================================================================"
echo ""
echo "  IPA: $IPA"
echo ""
echo "  ── Install on iPad ─────────────────────────────────────────────────"
echo "  Option A (connected via USB):"
echo "    xcrun devicectl device install app --device <UDID> \"$IPA\""
echo "    (find UDID in Xcode → Window → Devices & Simulators)"
echo ""
echo "  Option B: drag the IPA onto your device in Xcode's"
echo "            Devices & Simulators window"
echo ""
echo "  ── After installing ────────────────────────────────────────────────"
echo "  1. Trust developer profile:"
echo "     iPad Settings → General → VPN & Device Management → [your name] → Trust"
echo "  2. Open AUM → tap + → Audio Unit Extensions → Umaru Industrial Waste → Dan"
echo ""
echo "  ── Distribute via TestFlight ────────────────────────────────────────"
echo "  Re-export with method=app-store and upload via Xcode Organizer or"
echo "  xcrun altool / notarytool."
