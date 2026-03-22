; Dan VST/CLAP/Standalone installer — built by package-win.ps1
; Requires NSIS 3.x (https://nsis.sourceforge.io)
;
; Called with:
;   makensis /DVERSION=0.1.0 /DSRC_DIR=<absolute-path-to-staging> installer-win.nsi

!ifndef VERSION
  !define VERSION "0.1.0"
!endif
!ifndef SRC_DIR
  !define SRC_DIR "dist\_staging-win"
!endif

!define APP_NAME  "Dan"
!define PUBLISHER "Umaru Industrial Waste"
!define DIST_DIR  "dist"

Unicode true
Name            "${APP_NAME} ${VERSION}"
OutFile         "${DIST_DIR}\Dan-${VERSION}-Windows-Setup.exe"
InstallDir      "$PROGRAMFILES64\${APP_NAME}"
RequestExecutionLevel admin
SetCompressor   /SOLID lzma

!include "MUI2.nsh"
!include "Sections.nsh"

; ── Pages ─────────────────────────────────────────────────────────────────────
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

; ── Sections ──────────────────────────────────────────────────────────────────
Section "VST3 Plug-in" SecVST3
    SectionIn RO   ; always installed
    SetOutPath "$COMMONFILES64\VST3"
    File /r "${SRC_DIR}\VST3\Dan.vst3"
    ; Register for uninstall
    WriteRegStr HKLM "Software\${PUBLISHER}\${APP_NAME}" "VST3Path" "$COMMONFILES64\VST3\Dan.vst3"
SectionEnd

Section "CLAP Plug-in" SecCLAP
    SetOutPath "$COMMONFILES64\CLAP"
    File "${SRC_DIR}\CLAP\Dan.clap"
    WriteRegStr HKLM "Software\${PUBLISHER}\${APP_NAME}" "CLAPPath" "$COMMONFILES64\CLAP\Dan.clap"
SectionEnd

Section "Standalone App" SecStandalone
    SetOutPath "$INSTDIR"
    File "${SRC_DIR}\Standalone\Dan.exe"

    ; Start Menu shortcut
    CreateDirectory "$SMPROGRAMS\${PUBLISHER}"
    CreateShortcut  "$SMPROGRAMS\${PUBLISHER}\${APP_NAME}.lnk" "$INSTDIR\Dan.exe"

    ; Desktop shortcut
    CreateShortcut  "$DESKTOP\${APP_NAME}.lnk" "$INSTDIR\Dan.exe"

    WriteRegStr HKLM "Software\${PUBLISHER}\${APP_NAME}" "StandalonePath" "$INSTDIR\Dan.exe"
SectionEnd

; ── Uninstaller ───────────────────────────────────────────────────────────────
Section "-Write uninstaller"
    WriteUninstaller "$INSTDIR\Uninstall.lnk"
    WriteRegStr HKLM \
        "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" \
        "DisplayName" "${APP_NAME} ${VERSION} (${PUBLISHER})"
    WriteRegStr HKLM \
        "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" \
        "UninstallString" "$INSTDIR\Uninstall.lnk"
    WriteRegStr HKLM \
        "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" \
        "Publisher" "${PUBLISHER}"
    WriteRegStr HKLM \
        "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" \
        "DisplayVersion" "${VERSION}"
SectionEnd

Section "Uninstall"
    RMDir /r "$COMMONFILES64\VST3\Dan.vst3"
    Delete   "$COMMONFILES64\CLAP\Dan.clap"
    Delete   "$INSTDIR\Dan.exe"
    Delete   "$INSTDIR\Uninstall.lnk"
    RMDir    "$INSTDIR"
    Delete   "$SMPROGRAMS\${PUBLISHER}\${APP_NAME}.lnk"
    RMDir    "$SMPROGRAMS\${PUBLISHER}"
    Delete   "$DESKTOP\${APP_NAME}.lnk"
    DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}"
    DeleteRegKey HKLM "Software\${PUBLISHER}\${APP_NAME}"
SectionEnd

; ── Section descriptions ──────────────────────────────────────────────────────
!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${SecVST3}      "VST3 plug-in → $COMMONFILES64\VST3\"
    !insertmacro MUI_DESCRIPTION_TEXT ${SecCLAP}      "CLAP plug-in → $COMMONFILES64\CLAP\"
    !insertmacro MUI_DESCRIPTION_TEXT ${SecStandalone} "Standalone app → $PROGRAMFILES64\Dan\"
!insertmacro MUI_FUNCTION_DESCRIPTION_END
