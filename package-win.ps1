#Requires -Version 5.1
# Create a Windows distribution package for Dan.
# Produces a ZIP (always) and a Setup.exe (if NSIS makensis.exe is in PATH).
#
# Usage:
#   .\package-win.ps1
#   .\package-win.ps1 -BuildDir build-win -Version 0.1.0

param(
    [string]$BuildDir = "build-win",
    [string]$Version  = "0.1.0"
)

$ErrorActionPreference = "Stop"

$ARTEFACTS = "$BuildDir\MidiEnvelopePlugin_artefacts\Release"
$DIST      = "dist"
$STAGING   = "$DIST\_staging-win"

Write-Host "==> Packaging Dan $Version for Windows..."

# ── Stage artefacts ───────────────────────────────────────────────────────────
if (Test-Path $STAGING) { Remove-Item $STAGING -Recurse -Force }
New-Item -ItemType Directory -Path $STAGING | Out-Null

function Stage-Item {
    param([string]$Src, [string]$DestDir)
    $name = Split-Path $Src -Leaf
    if (-not (Test-Path $Src)) {
        Write-Host "  [skip] $name not found in build artefacts"
        return
    }
    New-Item -ItemType Directory -Path $DestDir -Force | Out-Null
    Copy-Item $Src "$DestDir\" -Recurse -Force
    Write-Host "  Staged $name"
}

# VST3 installs to %CommonProgramFiles%\VST3\
Stage-Item "$ARTEFACTS\VST3\Dan.vst3"       "$STAGING\VST3"
# CLAP installs to %CommonProgramFiles%\CLAP\
Stage-Item "$ARTEFACTS\CLAP\Dan.clap"       "$STAGING\CLAP"
# Standalone — placed at root of the zip / into Program Files\Dan\ by the installer
Stage-Item "$ARTEFACTS\Standalone\Dan.exe"  "$STAGING\Standalone"

# ── Write install instructions ────────────────────────────────────────────────
@"
Dan $Version — Windows
======================

Manual installation
-------------------
VST3:       Copy  Dan.vst3\  →  C:\Program Files\Common Files\VST3\
CLAP:       Copy  Dan.clap   →  C:\Program Files\Common Files\CLAP\
Standalone: Copy  Dan.exe    →  anywhere you like

If you ran the Setup.exe installer these were placed automatically.

Publisher: Umaru Industrial Waste
"@ | Out-File -FilePath "$STAGING\README.txt" -Encoding UTF8

New-Item -ItemType Directory -Path $DIST -Force | Out-Null

# ── ZIP ───────────────────────────────────────────────────────────────────────
$ZIP = "$DIST\Dan-$Version-Windows.zip"
if (Test-Path $ZIP) { Remove-Item $ZIP }
Compress-Archive -Path "$STAGING\*" -DestinationPath $ZIP
Write-Host ""
Write-Host "==> ZIP: $ZIP"

# ── NSIS installer (optional — requires makensis in PATH) ────────────────────
$makensis = Get-Command makensis -ErrorAction SilentlyContinue
if ($makensis) {
    Write-Host "==> Building NSIS installer..."
    & makensis /DVERSION=$Version /DSRC_DIR="$((Resolve-Path $STAGING).Path)" installer-win.nsi
    Write-Host "==> Installer: $DIST\Dan-$Version-Windows-Setup.exe"
} else {
    Write-Host ""
    Write-Host "    (NSIS not found — ZIP only.)"
    Write-Host "    For a Setup.exe: install NSIS from https://nsis.sourceforge.io"
    Write-Host "    then re-run this script."
}

# ── Cleanup staging ───────────────────────────────────────────────────────────
Remove-Item $STAGING -Recurse -Force
